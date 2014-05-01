#pragma once

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class data_length : public super
{
public:
    data_length(T_arg_list args = {})
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
        set_packet_field<T_data_len>(buf, buf->data_len);

#ifdef VERBOSE
        printf("Added data length=%hu for packet of %lu bytes\n", buf->data_len, buf->len);
#endif

        return super::read_pkt(buf);
    }
    
    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

        buf->data_len = get_packet_field<T_data_len>(buf);

#ifdef VERBOSE
        printf("Data length=%hu for packet of %lu bytes\n", buf->data_len, buf->len);
#endif
        return buf->len;
    }
};
