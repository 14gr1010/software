#pragma once

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class write_export : public super
{
    T_pkt_queue* output_buffers;

public:
    write_export(T_arg_list args = {})
    : super(args)
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
        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

        output_buffers->push_back(copy_pkt_buffer(buf));

#ifdef VERBOSE
        printf("Exported packet of of %lu bytes\n", buf->len);
#endif
        return buf->len;
    }

    void set_export_buffer(T_pkt_queue* queue)
    {
        output_buffers = queue;
    }
};
