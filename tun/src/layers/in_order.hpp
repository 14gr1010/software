#pragma once

#include <map> // std::map
#include <algorithm> // std::max(), std::min()
#include <deque> // std::deque

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class in_order : public super
{
private:
    // Transmitter
    T_pkt_id id = 0;

    // Receiver
    T_pkt_id next_id = 1;
    T_pkt_id last_fw_id = 0;
    std::map<T_pkt_id, p_pkt_buffer> rx_pkt_queue;
    double pkt_timeout = 0.0015;
    const int max_inter_pkt_delays = 15;
    std::deque<uint64_t> inter_pkt_delays;
    int inter_pkt_delays_index = 0;
    bool use_fixed_timeout = true;

public:
    in_order(T_arg_list args = {})
    : super(args)
    {
        parse(args, false);
        inter_pkt_delays.resize(max_inter_pkt_delays);
    }

    ~in_order()
    {
#ifdef VERBOSE
        printf("In order: size=%lu\n", rx_pkt_queue.size());
#endif
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        pkt_timeout = get_arg("in_order::timeout", pkt_timeout, args).d;
        use_fixed_timeout = get_arg("in_order::use_fixed_timeout", use_fixed_timeout, args).b;
        ASSERT_GT(pkt_timeout, 0);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        buf->id = generate_pkt_id();
        set_packet_field<T_pkt_id>(buf, buf->id);

        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            if (next_pkt_ready())
            {
                rx_deliver_next_pkt(buf);
                return buf->len;
            }
            else if (first_pkt_time_out())
            {
                last_fw_id = rx_pkt_queue.begin()->first-1;
                rx_deliver_next_pkt(buf);
                return buf->len;
            }
        }
        WRITE(super, buf);

        if (use_fixed_timeout == false)
        {
            update_rx_pkt_timeout();
        }

        buf->id = get_packet_field<T_pkt_id>(buf);

        if (buf->id <= last_fw_id)
        {
#ifdef VERBOSE
            printf("Discarding (to late) pkt of %lu bytes and id %u, last_fw_id %u\n", buf->len, buf->id, last_fw_id);
#endif
            buf->clear();
            return 0;
        }

        if (buf->id == next_id)
        {
            ++next_id;
        }

        if (last_fw_id + 1 == buf->id)
        {
#ifdef VERBOSE
            printf("Forwading pkt of %lu bytes and id %u\n", buf->len, buf->id);
#endif
            ++last_fw_id;
            return buf->len;
        }

        rx_store_pkt(buf);

        if (next_pkt_ready())
        {
            rx_deliver_next_pkt(buf);
        }

        garbage_collection();

        return buf->len;
    }

private:
    T_pkt_id generate_pkt_id()
    {
        return ++id;
    }

    void update_rx_pkt_timeout()
    {
        static uint64_t now = 0;
        static uint64_t last = 0;
        last = now;
        now = time_stamp();
        inter_pkt_delays[inter_pkt_delays_index] = now - last;
        inter_pkt_delays_index =
            (inter_pkt_delays_index + 1) % max_inter_pkt_delays;

        uint64_t total_inter_pkt_delay = 0;
        for (uint64_t i : inter_pkt_delays)
        {
            total_inter_pkt_delay += std::min(std::max(i, 20UL), 500UL);
        }
        pkt_timeout =
            total_inter_pkt_delay/((double)max_inter_pkt_delays*1000000);
        pkt_timeout *= 1.05;
    }

    bool next_pkt_ready() const
    {
        return (rx_pkt_queue.count(last_fw_id+1) != 0);
    }

    bool first_pkt_time_out()
    {
        if (rx_pkt_queue.size() == 0)
        {
            return false;
        }
        uint64_t time_now = time_stamp();
        return (rx_pkt_queue.begin()->second->time_stamp + (1000000 * pkt_timeout) <= time_now);
    }

    void rx_store_pkt(p_pkt_buffer& buf)
    {
#ifdef VERBOSE
        printf("Storing pkt of %lu bytes and id %u\n", buf->len, buf->id);
#endif
        if (rx_pkt_queue.count(buf->id) == 0)
        {
            buf->time_stamp = time_stamp();
            rx_pkt_queue[buf->id] = buf;
            buf = new_pkt_buffer();
        }
    }

    void rx_deliver_next_pkt(p_pkt_buffer& buf)
    {
        ASSERT_NEQ(rx_pkt_queue.count(last_fw_id+1), 0);

        buf->clear();
        ++last_fw_id;
        buf = rx_pkt_queue[last_fw_id];
        rx_pkt_queue.erase(last_fw_id);
#ifdef VERBOSE
        printf("Delivering pkt of %lu bytes and id %u\n", buf->len, buf->id);
#endif
    }

    void garbage_collection()
    {
        for (auto i = rx_pkt_queue.cbegin(); i != rx_pkt_queue.cend(); /* NO INCREMENT*/)
        {
            if (i->second->id <= last_fw_id)
            {
#ifdef VERBOSE
                printf("Erasing pkt with id=%u, last_fw_id %u\n", i->first, last_fw_id);
#endif
                rx_pkt_queue.erase(i++);
            }
            else
            {
                ++i;
            }
        }
    }
};
