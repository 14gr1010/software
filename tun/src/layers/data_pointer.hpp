#pragma once

#include "misc.hpp"
#include "pkt_buffer.hpp"

typedef uint8_t T_hdr_len;

static auto set_packet_header_length = set_packet_field<T_hdr_len>;
static auto get_packet_header_length = get_packet_field<T_hdr_len>;

template<class super>
class data_pointer : public super
{
public:
    data_pointer(T_arg_list args = {})
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
        T_hdr_len len = buf->data - buf->head;
        set_packet_header_length(buf, len);
#ifdef VERBOSE
        printf("Added header length field, with value: %u\n", len);
#endif

        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);

        T_hdr_len len = get_packet_header_length(buf);
        buf->data = buf->head + len;

#ifdef VERBOSE
        printf("Received header length field, with value: %u\n", len);
#endif
        return buf->len;
    }
};
