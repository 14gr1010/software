#pragma once

#include <linux/ip.h> // iphdr

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class protocol_id : public super
{
public:
    protocol_id(T_arg_list args = {})
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
        set_packet_field<uint8_t>(buf, buf->protocol);
        
#ifdef VERBOSE
        printf("Added protocol ID=%d for packet of %lu bytes\n", buf->protocol, buf->len);
#endif

        return super::read_pkt(buf);
    }
    
    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

        buf->protocol = get_packet_field<uint8_t>(buf);

#ifdef VERBOSE
        printf("Protocol ID=%d for packet of %lu bytes\n", buf->protocol, buf->len);
#endif
        return buf->len;
    }
};
