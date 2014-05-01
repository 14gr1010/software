#pragma once

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class no_redundancy : public super
{
public:
    no_redundancy(T_arg_list args = {})
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
        printf("No redundancy added, for packet of %lu bytes\n", buf->len);
#endif

        return super::read_pkt(buf);
    }
    
    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

#ifdef VERBOSE
        printf("No redundancy expected, for packet of %lu bytes\n", buf->len);
#endif
        return buf->len;
    }
};
