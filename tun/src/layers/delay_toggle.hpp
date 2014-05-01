#pragma once

#include <vector> // std::vector

#include "misc.hpp"
#include "pkt_buffer.hpp"
#include "delay.hpp"

template<class super>
class delay_toggle : public delay<super>
{
    typedef delay<super> delayer;

    // Transmitter
    std::vector<double> tx_delay;
    size_t tx_index = 0;

public:
    delay_toggle(T_arg_list args = {})
    : delayer(args)
    {
        tx_delay.reserve(2);
        tx_delay.resize(1);
        tx_delay[0] = 0.001;
        parse(args, false);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            delayer::parse(args);
        }
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        return delayer::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (!buf->is_empty())
        {
            tx_index = (tx_index+1) % tx_delay.size();
            double new_delay = tx_delay[tx_index];
            delayer::parse({ {"delay::tx", new_delay} }, false);

#ifdef VERBOSE
            printf("TX delay set to %f s, index %lu\n", new_delay, tx_index);
#endif
        }
        WRITE(delayer, buf);

        return buf->len;
    }

    void set_tx_delay(size_t index, double new_delay)
    {
        ASSERT_GT(new_delay, 0);

        if (index >= tx_delay.size())
        {
            tx_delay.resize(index+1);
        }

        tx_delay[index] = new_delay;
    }
};
