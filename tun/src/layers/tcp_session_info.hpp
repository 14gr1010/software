#pragma once

#include <cstdlib> // size_t
#include <linux/ip.h> // iphdr
#include <linux/tcp.h> // tcphdr
#include <netdb.h> // getprotobyname()

#include "misc.hpp"
#include "pkt_buffer.hpp"

#define NORMALISE_TCP_SEQUENCE_NUMBER

struct t_stat
{
    uint64_t time_stamp;
    uint32_t stat;
};

struct t_stats
{
    std::deque<t_stat> pkt_loss;
    std::deque<t_stat> window;
};

struct tcp_session_id
{
    uint32_t ip_src;
    uint32_t ip_dst;
    uint16_t sport;
    uint16_t dport;
};

struct tcp_session
{
    uint64_t time_stamp;
    uint32_t next_seq;
    uint32_t start_seq;
    uint32_t window_size;
    t_stats stats;
    std::map<uint32_t, uint16_t> ack_status;
};

inline bool operator<(const tcp_session_id& l, const tcp_session_id& r)
{
    if (l.ip_src < r.ip_src)
        return true;
    if (l.ip_src > r.ip_src)
        return false;

    if (l.ip_dst < r.ip_dst)
        return true;
    if (l.ip_dst > r.ip_dst)
        return false;

    if (l.sport < r.sport)
        return true;
    if (l.sport > r.sport)
        return false;

    if (l.dport < r.dport)
        return true;
    return false;
}

inline void print_ip(char *str, uint32_t ip)
{
    sprintf(str, "%u.%u.%u.%u", (ip >> 24) & 0xFF,
           (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip >> 0) & 0xFF);
}

template<class super>
class tcp_session_info : public super
{
    const int tcp_protocol_id;

    std::map<tcp_session_id, tcp_session> tcp_sessions;
    size_t active_sessions_max = 0;
    size_t active_sessions_total = 0;

public:
    tcp_session_info(T_arg_list args = {})
    : super(args),
      tcp_protocol_id(getprotobyname("tcp")->p_proto)
    {
        parse(args, false);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        iphdr *iph = (iphdr*)(buf->data);

        if (tcp_protocol_id == iph->protocol)
        {
            handle_tcp_pkt(buf);
        }

        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

        iphdr *iph = (iphdr*)(buf->data);

        if (tcp_protocol_id == iph->protocol)
        {
            handle_tcp_pkt(buf);
        }

        return buf->len;
    }

private:
    void handle_tcp_pkt(p_pkt_buffer& buf)
    {
        iphdr *iph = (iphdr*)buf->data;
        uint32_t ip_len = ntohs(iph->tot_len);
        uint32_t ip_hlen = iph->ihl*4;
//        uint8_t version = iph->version;

        tcphdr *tcph = (tcphdr*)(buf->data + ip_hlen);

        tcp_session_id id = get_session_id(iph, tcph);
        bool is_active = is_active_session(id);
        uint32_t tcp_next_seq = 0;
        if (is_active)
        {
            tcp_next_seq = tcp_sessions[id].next_seq;
        }
        uint16_t tcp_hlen = 4*tcph->doff;
        uint16_t tcp_data_len = ip_len-ip_hlen-tcp_hlen;
        uint32_t tcp_seq = ntohl(tcph->seq);
        uint32_t tcp_ack = ntohl(tcph->ack_seq);

#ifdef NORMALISE_TCP_SEQUENCE_NUMBER
        if (is_active)
        {
            tcp_seq -= tcp_sessions[id].start_seq;
        }
        tcp_session_id swaped = swap_src_dst(id);
        if (is_active_session(swaped))
        {
            tcp_ack -= tcp_sessions[swaped].start_seq;
        }
#endif

        if (tcph->syn == 1)
        {
            store_tcp_session(buf);
#ifdef NORMALISE_TCP_SEQUENCE_NUMBER
            tcp_seq = 0;
            tcp_sessions[id].next_seq = 1;
#endif
        }
        else if (tcph->fin == 1 || tcph->rst == 1)
        {
            terminate_tcp_session(id);
            return;
        }
        else if (is_active)
        {
            if (tcp_seq > tcp_next_seq)
            {
                uint32_t data_lost = tcp_seq - tcp_next_seq;
                uint32_t packets_lost;
                if (tcp_data_len > 0)
                {
                    packets_lost = data_lost/tcp_data_len;
                }
                else
                {
                    packets_lost = data_lost/1350;
                }
#ifdef VERBOSE
                printf("Lost %u bytes (~%u pkts). Expected seq to be %u but seq is %u\n", data_lost, packets_lost, tcp_next_seq, tcp_seq);
#endif
                t_stat new_stat = {time_stamp(), packets_lost};
                tcp_sessions[id].stats.pkt_loss.push_back(new_stat);
                tcp_sessions[id].next_seq = tcp_seq + tcp_data_len;
            }
            else if (tcp_seq < tcp_next_seq)
            {
#ifdef VERBOSE
                printf("Received %hu bytes out-of-order, with seq: %u\n", tcp_data_len, tcp_seq);
#endif
            }
            else
            {
                tcp_sessions[id].next_seq += tcp_data_len;
            }

            update_tcp_window_sack(id, tcph);
            update_tcp_window(id, tcp_data_len, tcp_seq, tcp_ack);
        }
        else
        {
            return;
        }
    }

    void update_tcp_window_sack(tcp_session_id id, tcphdr *tcph)
    {
        uint16_t tcp_hlen = 4*tcph->doff;
        uint8_t *options = (uint8_t*)(tcph) + sizeof(tcphdr);
        uint16_t options_length = tcp_hlen - sizeof(tcphdr);

        tcp_session_id swaped = swap_src_dst(id);

        for (uint16_t i = 0; i < options_length; )
        {
            uint8_t byte = options[i];
            if (byte == 1)
            {
                ++i;
                continue;
            }
            uint8_t len = options[++i];
            if (byte != 5)
            {
                i += len - 1;
                continue;
            }
            uint64_t time = time_stamp();

            uint8_t sacks = (len - 2) / 8;
            ++i;
            for (uint8_t u = 0; u < sacks; ++u)
            {
                uint32_t begin = ntohl(*(uint32_t*)&options[i]);
                i += 4;
                uint32_t end = ntohl(*(uint32_t*)&options[i]);
                i += 4;
                update_tcp_window_single_sack(swaped, begin, end);
            }

            tcp_sessions[swaped].stats.window.push_back({time, tcp_sessions[swaped].window_size});
        }
    }

    void update_tcp_window_single_sack(tcp_session_id swaped, uint32_t begin, uint32_t end)
    {
#ifdef NORMALISE_TCP_SEQUENCE_NUMBER
        begin -= tcp_sessions[swaped].start_seq;
        end -= tcp_sessions[swaped].start_seq;
#endif
        // for (auto i = tcp_sessions[swaped].ack_status.begin();
        //      i != tcp_sessions[swaped].ack_status.end(); ++i)
        for (auto i : tcp_sessions[swaped].ack_status)
        {
            if (i.second == 0)
            {
                continue;
            }
            if ((i.first >= begin) &&
                (i.first < end))
            {
#ifdef PRINT_SACK_SEQ_AND_SIZE
                printf("SACK %u bytes with seq %u\n", i.second, i.first);
#endif
                tcp_sessions[swaped].window_size -= i.second;
                i.second = 0;
            }
        }
    }

    void update_tcp_window(tcp_session_id id, uint16_t tcp_data_len, uint32_t seq, uint32_t ack)
    {
        uint64_t time = time_stamp();

        tcp_session_id swaped = swap_src_dst(id);
        if (is_active_session(swaped))
        {
            // for (auto i = tcp_sessions[swaped].ack_status.begin();
            //      i != tcp_sessions[swaped].ack_status.end(); ++i)
            for (auto i : tcp_sessions[swaped].ack_status)
            {
                if (i.second == 0)
                {
                    continue;
                }
                if (i.first < ack)
                {
                    tcp_sessions[swaped].window_size -= i.second;
                    i.second = 0;
                }
            }
            tcp_sessions[swaped].stats.window.push_back({time, tcp_sessions[swaped].window_size});
        }

        if (tcp_data_len == 0)
        {
            return;
        }

        if (tcp_sessions[id].ack_status.count(seq) == 0)
        {
            tcp_sessions[id].ack_status[seq] = tcp_data_len;
            tcp_sessions[id].window_size += tcp_data_len;
            tcp_sessions[id].stats.window.push_back({time, tcp_sessions[id].window_size});
        }
    }

    void store_tcp_session(p_pkt_buffer& buf)
    {
        iphdr *iph = (iphdr*)buf->data;
        uint32_t ip_hlen = iph->ihl*4;
        tcphdr *tcph = (tcphdr*)(buf->data + ip_hlen);

        tcp_session new_session;
        new_session.time_stamp = time_stamp();
        new_session.start_seq = ntohl(tcph->seq);
        new_session.next_seq = new_session.start_seq+1;
        new_session.window_size = 0;
        tcp_session_id id = get_session_id(iph, tcph);

        if (!is_active_session(id))
        {
            tcp_sessions[id] = new_session;
        }
        else
        {
#ifdef VERBOSE
            uint64_t time_diff = time_stamp() - tcp_sessions[id].time_stamp;
            char ip_src[16];
            char ip_dst[16];
            print_ip(ip_src, id.ip_src);
            print_ip(ip_dst, id.ip_dst);
            printf("TCP connection between: %s:%hu and %s:%hu initialised %lu s (%lu us) ago\n",
                   ip_src, id.sport, ip_dst, id.dport, time_diff/1000000, time_diff);
#endif
            terminate_tcp_session(id);
            store_tcp_session(buf);
            return;
        }

#ifdef VERBOSE
        char ip_src[16];
        char ip_dst[16];
        print_ip(ip_src, id.ip_src);
        print_ip(ip_dst, id.ip_dst);
        printf("Initialised TCP connection between: %s:%hu and %s:%hu\n",
               ip_src, id.sport, ip_dst, id.dport);
#endif
        active_sessions_max = std::max(active_sessions_max, tcp_sessions.size());
        ++active_sessions_total;
    }

    void terminate_tcp_session(tcp_session_id id)
    {
        if (is_active_session(id))
        {
            tcp_sessions.erase(id);
#ifdef VERBOSE
            char ip_src[16];
            char ip_dst[16];
            print_ip(ip_src, id.ip_src);
            print_ip(ip_dst, id.ip_dst);
            printf("Terminated TCP connection between: %s:%hu and %s:%hu\n",
                   ip_src, id.sport, ip_dst, id.dport);
#endif
        }
    }

    tcp_session_id get_session_id(iphdr *iph, tcphdr *tcph)
    {
        tcp_session_id id = {ntohl(iph->saddr),
                             ntohl(iph->daddr),
                             ntohs(tcph->source),
                             ntohs(tcph->dest)};
        return id;
    }

    bool is_active_session(tcp_session_id id)
    {
        if (0 == tcp_sessions.count(id))
        {
            return false;
        }
        return true;
    }

    tcp_session_id swap_src_dst(tcp_session_id old)
    {
        tcp_session_id id = {old.ip_dst,
                             old.ip_src,
                             old.dport,
                             old.sport};
        return id;
    }
};
