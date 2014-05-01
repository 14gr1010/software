#pragma once

#include <map> // std::map

#include "misc.hpp"
#include "pkt_buffer.hpp"
#include "data_length.hpp"
#include "data_pointer.hpp"
#include "final_layer.hpp"

class split_data_length : public final_layer
{
public:
    typedef data_pointer<data_length<final_layer>> substack;

private:
    std::map<T_data_len, substack*> stacks;

public:
    split_data_length(T_arg_list args = {})
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

        buf->data_len = buf->len;
        ASSERT_GTE(buf->len, 20);

        for (auto i : stacks)
        {
            if (i.first >= buf->data_len)
            {
#if defined(VERBOSE) || defined(SPLIT_DATA_LENGTH_VERBOSE)
                printf("Packet of %lu bytes sendt via stack %u (max data length)\n", buf->len, i.first);
#endif
                return i.second->read_pkt(buf);
            }
        }

        if (stacks.count(0))
        {
#if defined(VERBOSE) || defined(SPLIT_DATA_LENGTH_VERBOSE)
            printf("Packet of %lu bytes sendt via stack 0 (default stack)\n", buf->len);
#endif
            return stacks[0]->read_pkt(buf);
        }

#if defined(VERBOSE) || defined(SPLIT_DATA_LENGTH_VERBOSE)
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

        T_data_len data_len = read_packet_field<T_data_len>(buf);
        ASSERT_GTE(data_len, 20);
        bool used = false;

        for (auto i : stacks)
        {
            if (i.first >= data_len)
            {
#if defined(VERBOSE) || defined(SPLIT_DATA_LENGTH_VERBOSE)
                printf("Packet of %lu bytes written to stack %u (max data length)\n", buf->len, i.first);
#endif
                used = true;
                return i.second->write_pkt(buf);
            }
        }

        if (used == false && stacks.count(0) > 0)
        {
#if defined(VERBOSE) || defined(SPLIT_DATA_LENGTH_VERBOSE)
            printf("Packet of %lu bytes written to stack 0 (default stack)\n", buf->len);
#endif
            return stacks[0]->write_pkt(buf);
        }

#if defined(VERBOSE) || defined(SPLIT_DATA_LENGTH_VERBOSE)
        printf("ERROR: No stack for packet of %lu bytes\n", buf->len);
#endif
        buf->print_head(buf->len, "Buffer:");

        buf->clear();

        return 0;
    }

    void set_stack(substack* stack, T_data_len max_length)
    {
        ASSERT_EQ(stacks.count(max_length), 0);
        stacks[max_length] = stack;
    }
};
