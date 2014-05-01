#pragma once

#include <linux/ip.h> // iphdr
#include <map> // std::map

#include "misc.hpp"
#include "pkt_buffer.hpp"
#include "protocol_id.hpp"
#include "data_pointer.hpp"
#include "final_layer.hpp"

class split_protocols : public final_layer
{
public:
    typedef data_pointer<protocol_id<final_layer>> substack;

private:
    std::map<uint8_t, substack*> stacks;

public:
    split_protocols(T_arg_list args = {})
    : final_layer(args)
    {
        parse(args, false);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            for (auto i : stacks)
            {
                i.second->parse(args);
            }
        }
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            return 0;
        }

        iphdr *iph = (iphdr*)(buf->data);
        buf->protocol = iph->protocol;
        ASSERT_NEQ(buf->protocol, 0);

        for (auto i : stacks)
        {
            if (i.first == buf->protocol)
            {
#ifdef VERBOSE
                printf("Packet of %lu bytes sendt via stack %u (IP protocol ID)\n", buf->len, i.first);
#endif
                return i.second->read_pkt(buf);
            }
        }

        if (stacks.count(0))
        {
#ifdef VERBOSE
            printf("Packet of %lu bytes sendt via stack 0 (default stack)\n", buf->len);
#endif
            return stacks[0]->read_pkt(buf);
        }

#ifdef VERBOSE
        printf("ERROR: No stack for packet of %lu bytes\n", buf->len);
#endif
        buf->print_head(buf->len, "Buffer:");

        buf->clear();

        return 0;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            for (auto i : stacks)
            {
                int n = i.second->write_pkt(buf);
                if (n > 0)
                {
                    return n;
                }
            }
            return 0;
        }

        uint8_t protocol = read_packet_field<uint8_t>(buf);
        ASSERT_NEQ(protocol, 0);
        bool used = false;

        for (auto i : stacks)
        {
            if (i.first == protocol)
            {
#ifdef VERBOSE
                printf("Packet of %lu bytes written to stack %u (IP protocol ID)\n", buf->len, i.first);
#endif
                used = true;
                return i.second->write_pkt(buf);
            }
        }

        if (used == false && stacks.count(0))
        {
#ifdef VERBOSE
            printf("Packet of %lu bytes written to stack 0 (default stack)\n", buf->len);
#endif
            return stacks[0]->write_pkt(buf);
        }

#ifdef VERBOSE
        printf("ERROR: No stack for packet of %lu bytes\n", buf->len);
#endif
        buf->print_head(buf->len, "Buffer:");

        buf->clear();

        return 0;
    }

    void set_stack(substack* stack, uint8_t protocol_id)
    {
        ASSERT_EQ(stacks.count(protocol_id), 0);
        stacks[protocol_id] = stack;
    }
};
