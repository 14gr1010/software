#pragma once

#include <cstdio> // printf()
#include <vector> // std::vector
#include <map> // std::map
#include <fstream> // std::fstream

#include "misc.hpp"
#include "pkt_buffer.hpp"
#include "xor_redundancy.hpp"

template<class super>
class xor_fixed_redundancy : public super
{
    // Encode info
    uint8_t xor_amount = 5; // Amount of packets to XOR together, 0 = no redundancy
    std::vector<T_pkt_id_len> xor_info;
    p_pkt_buffer xor_data;
    T_pkt_id tx_xor_id = 0;
    T_pkt_id tx_id = 0;

    // Decode info
    T_pkt_id seen_pkt = 0;
    int forwarded_pkts = 0;
    std::map<T_pkt_id, p_pkt_buffer> in_pkt; // Received uncoded packets + decoded packets
    std::map<T_pkt_id, p_pkt_buffer> rx_xor_pkts; // Map of undecodable XOR packets
    std::map<T_pkt_id, std::vector<T_pkt_id_len>> rx_xor_info; // Map of ID+length of the missing packets for each stored XOR packet
    std::map<T_pkt_id, T_pkt_id> rx_xor_missing_pkts; // Map of the missing packets, with ID of the XOR which is missing them
    T_pkt_queue pkts_ready; // Fifo queue for decoded packets (only used when a coded packet is not decodable when received by is decodable later)
    const size_t garbage_collection_safety_limit = 20;

public:
    xor_fixed_redundancy(T_arg_list args = {})
    : super(args)
    {
        xor_data = new_pkt_buffer();
        parse(args, false);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        xor_amount = get_arg("xor_fixed_redundancy::amount", xor_amount, args).u8;
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        if (buf->id == 0)
        {
            buf->id = generate_pkt_id();
        }

        T_pkt_len len = buf->len;
        T_pkt_id id = buf->id;

        ASSERT_NEQ(id, 0);

        if (xor_amount > 0)
        {
            xor_data->xor_in(buf);
            xor_info.push_back({id, len});
        }

        bool redundancy_ready = false;

        if (xor_info.size() >= xor_amount && xor_info.size() != 0)
        {
            redundancy_ready = true;

            ASSERT_EQ(xor_data->head, xor_data->data);
            ASSERT_GT(xor_info.size(), 0);
            for (auto i : xor_info)
            {
                set_packet_len(xor_data, i.len);
                set_packet_id(xor_data, i.id);
                xor_data->data_len = std::max(xor_data->data_len, i.len);
            }

            xor_data->id = --tx_xor_id;
            xor_data->protocol = buf->protocol;

            set_packet_id(xor_data, xor_data->id);
            set_packet_xors(xor_data, xor_info.size());
            set_packet_type(xor_data, REDUNDANCY_PKT);
        }

        set_packet_id(buf, id);
        set_packet_type(buf, SYSTEMATIC_PKT);

#ifdef VERBOSE
        printf("Sending a SYSTEMATIC packet on %lu bytes with id %u\n", buf->len, id);
#endif
        int n = super::read_pkt(buf);

        if (redundancy_ready)
        {
#ifdef VERBOSE
            printf("Sending a REDUNDANCY packet on %lu bytes with id %u\n", xor_data->len, xor_data->id);
#endif

            super::read_pkt(xor_data);

            xor_data->clear();
            xor_info.clear();
            xor_info.reserve(xor_amount);
        }

        return n;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            if (pkts_ready.size() > 0)
            {
                buf = pkts_ready.front();
                pkts_ready.pop_front();
                return buf->len;
            }
        }
        WRITE(super, buf);

        garbage_collection();

        T_pkt_redundancy type = get_packet_type(buf);

        if (type == SYSTEMATIC_PKT)
        {
            rx_handle_systematic_pkt(buf);
        }
        else if (type == REDUNDANCY_PKT)
        {
            rx_handle_redundancy_pkt(buf);
        }
        else
        {
            buf->print_head(buf->len);
            assert(0);
        }

        return buf->len;
    }

    uint8_t get_redundancy() const
    {
        return xor_amount;
    }

    double get_channel_success_rate() const
    {
        if (seen_pkt == 0)
        {
            return 1;
        }
        return forwarded_pkts / static_cast<double>(seen_pkt);
    }

    void print_decode_stat(std::fstream& fio)
    {
        ASSERT_EQ(fio.is_open(), true);
        fio << seen_pkt << "," << forwarded_pkts << "," << get_channel_success_rate() << ",";
    }

private:
    T_pkt_id generate_pkt_id()
    {
        return ++tx_id;
    }

    void rx_handle_systematic_pkt(p_pkt_buffer& buf)
    {
        T_pkt_id id = get_packet_id(buf);
        buf->id = id;

#ifdef VERBOSE
        printf("Received a SYSTEMATIC packet on %lu bytes with id %u\n", buf->len, id);
#endif
        if (in_pkt.count(id) != 0)
        {
#ifdef VERBOSE
            printf("SKIPPING SYSTEMATIC packet on %lu bytes with id %u\n", buf->len, id);
#endif
            // Packet being skipped, already forwarded
            buf->clear();
            return;
        }

        ++forwarded_pkts;
        seen_pkt = std::max(seen_pkt, id);
        in_pkt[id] = copy_pkt_buffer(buf);

        rx_handle_late_redundancy(buf->id);
    }

    void rx_handle_redundancy_pkt(p_pkt_buffer& buf)
    {
        uint8_t pkts = get_packet_xors(buf);
        T_pkt_id xor_id = get_packet_id(buf);
#ifdef VERBOSE
        printf("Received a REDUNDANCY packet on %lu bytes with xors %hu\n", buf->len, pkts);
#endif
        ASSERT_GT(pkts, 0);

        int unknown_pkts = pkts;
        T_pkt_id found_id = 0;
        T_pkt_len found_len = 0;
        std::vector<T_pkt_id_len> new_xor_info;
        new_xor_info.reserve(pkts);

        for (uint8_t n = 0; n < pkts; ++n)
        {
            T_pkt_id id = get_packet_id(buf);
            T_pkt_len len = get_packet_len(buf);
            new_xor_info.push_back({id, len});
#ifdef VERBOSE
            printf("Received redundant pkt for id: %u with len: %hu\n", id, len);
#endif
            if (in_pkt.count(id) != 0)
            {
                unknown_pkts--;
            }
            else
            {
                found_id = id;
                found_len = len;
            }
        }

        if (unknown_pkts == 0)
        {
#ifdef VERBOSE
            printf("SKIPPING REDUNDANCY packet on %lu bytes with 0 unknown pkts\n", buf->len);
#endif
            buf->clear();
        }
        else if (unknown_pkts == 1)
        {
            for (auto n : new_xor_info)
            {
                if (n.id == found_id)
                {
                    continue;
                }
                buf->xor_out(in_pkt[n.id], found_len);
            }
            buf->len = found_len; // Adjust length after XORing out pkts
            buf->id = found_id; // Save ID, for higher layers

            ++forwarded_pkts;
            seen_pkt = std::max(seen_pkt, found_id);
            in_pkt[found_id] = copy_pkt_buffer(buf);
#ifdef VERBOSE
            printf("Using REDUNDANCY packet on %lu bytes with ID %u\n", buf->len, found_id);
#endif
        }
        else
        {
            rx_xor_pkts[xor_id] = buf;
            buf = new_pkt_buffer();

            T_pkt_len max_len = 0;
            for (auto n : new_xor_info)
            {
                if (in_pkt.count(n.id) == 0)
                {
                    rx_xor_missing_pkts[n.id] = xor_id;
                    max_len = std::max(max_len, n.len);
                    rx_xor_info[xor_id].push_back(n);
                }
            }

            for (auto n : new_xor_info)
            {
                if (in_pkt.count(n.id) != 0)
                {
                    rx_xor_pkts[xor_id]->xor_out(in_pkt[n.id], max_len);
                }
            }
#ifdef VERBOSE
            printf("Saving REDUNDANCY packet on %lu bytes with ID %u and %u unknown pkts\n", buf->len, xor_id, unknown_pkts);
#endif
            while (rx_xor_pkts.size() > garbage_collection_safety_limit)
            {
                for (auto i = rx_xor_missing_pkts.begin();
                     i != rx_xor_missing_pkts.end(); /* NO INCREMENT*/)
                {
                    if (i->second == rx_xor_pkts.rbegin()->first)
                    {
                        rx_xor_missing_pkts.erase(i++);
                    }
                    else
                    {
                        ++i;
                    }
                }

                rx_xor_pkts.erase(--rx_xor_pkts.end());
                rx_xor_info.erase(--rx_xor_info.end());
            }
        }
    }

    // Handle previously received XOR packets,
    // when a new uncoded packet is received.
    // Input 'id' is the id of the newly received uncoded packet.
    void rx_handle_late_redundancy(T_pkt_id id)
    {
        if (rx_xor_missing_pkts.count(id) == 0)
        {
            return;
        }
        T_pkt_id xor_id = rx_xor_missing_pkts[id];
        rx_xor_missing_pkts.erase(id);

        for (size_t i = 0; i < rx_xor_info[xor_id].size(); ++i)
        {
            if (rx_xor_info[xor_id][i].id == id)
            {
                rx_xor_info[xor_id].erase(rx_xor_info[xor_id].begin()+i);
                break;
            }
        }

        T_pkt_len max_len = 0;
        for (auto i : rx_xor_info[xor_id])
        {
            max_len = std::max(max_len, i.len);
        }
        rx_xor_pkts[xor_id]->xor_out(in_pkt[id], max_len);

#ifdef VERBOSE
        printf("XORing REDUNDANCY packet on %lu bytes with ID %u with packet %u (max length %u bytes)\n", rx_xor_pkts[xor_id]->len, rx_xor_pkts[xor_id]->id, id, max_len);
#endif

        if (rx_xor_info[xor_id].size() > 1)
        {
            return;
        }
        rx_xor_pkts[xor_id]->id = rx_xor_info[xor_id][0].id;
        pkts_ready.push_back(rx_xor_pkts[xor_id]);
#ifdef VERBOSE
        printf("Adding REDUNDANCY packet on %lu bytes with ID %u to output queue\n", rx_xor_pkts[xor_id]->len, rx_xor_pkts[xor_id]->id);
#endif
        rx_xor_missing_pkts.erase(rx_xor_info[xor_id][0].id);
        rx_xor_pkts.erase(xor_id);
        rx_xor_info.erase(xor_id);
    }

    void garbage_collection()
    {
        size_t garbage_collection_limit = garbage_collection_safety_limit + xor_amount;

        if (seen_pkt < garbage_collection_limit)
        {
            return;
        }

        for (auto i = in_pkt.cbegin(); i != in_pkt.cend(); /* NO INCREMENT*/)
        {
            if (i->first < seen_pkt - garbage_collection_limit)
            {
#ifdef VERBOSE
                printf("Erasing pkt with id=%u\n", i->first);
#endif
                in_pkt.erase(i++);
            }
            else
            {
                ++i;
            }
        }
    }
};
