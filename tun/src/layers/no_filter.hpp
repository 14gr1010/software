#pragma once

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class no_filter : public super
{
public:
    no_filter(T_arg_list args = {})
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
#ifdef VERBOSE
        printf("Packet of %lu bytes NOT filtered\n", buf->len);
#endif

        return super::read_pkt(buf);
    }
    
    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

#ifdef VERBOSE
        printf("Packet of %lu bytes NOT filtered\n", buf->len);
#endif
        return buf->len;
    }
};
