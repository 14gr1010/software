#pragma once

#include <cstdlib> // size_t
#include <linux/ip.h> // iphdr
#include <netdb.h> // getprotobyname()

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class udp_filter : public super
{
    const int udp_protocol_id;

public:
    udp_filter(T_arg_list args = {})
    : super(args),
      udp_protocol_id(getprotobyname("udp")->p_proto)
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
        iphdr *iph = (iphdr*)buf->data;

        if (udp_protocol_id == iph->protocol)
        {
            return super::read_pkt(buf);
        }
#ifdef VERBOSE
            printf("filtered pkt (not UDP) with %lu bytes\n", buf->len);
#endif
        buf->clear();

        return 0;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);
        
        iphdr *iph = (iphdr*)(buf->data);

        if (udp_protocol_id == iph->protocol)
        {
            return buf->len;
        }
#ifdef VERBOSE
        printf("Filtered pkt (not TCP) with %lu bytes\n", buf->len);
#endif
        buf->clear();

        return 0;
    }
};
