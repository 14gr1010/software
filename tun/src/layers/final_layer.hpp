#pragma once

#include <boost/signals2.hpp> // namespace: boost::signals2
#include <fstream> // std::fstream

#include "pkt_buffer.hpp"

class final_layer
{
public:
    final_layer() = delete;

    final_layer(T_arg_list args = {})
    {
        parse(args, false);
    }

    virtual ~final_layer() {}

    virtual void parse(T_arg_list /* args */, bool /* call_super */ = true ) {}

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        size_t buflen = buf->len;
        if (signal_fw.num_slots() == 0)
        {
            return buflen;
        }
        
        ASSERT_EQ(signal_fw.num_slots(), 1);

        while (signal_fw(buf) != 0)
        {
            buf->clear();
        }
        
        return buflen;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        return buf->len;
    }

    void failure() {}

    void print_drop_stat()
    {
        assert(0);
    }

    void print_decode_stat(std::fstream& /* fio */)
    {
        assert(0);
    }


    typedef boost::signals2::signal<int (p_pkt_buffer&)> T_signal_fw;

    T_signal_fw signal_fw;

    void connect_signal_fw(const T_signal_fw::slot_type &slot)
    {
        signal_fw.connect(slot);
    }
};
