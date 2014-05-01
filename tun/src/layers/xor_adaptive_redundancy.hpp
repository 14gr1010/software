#pragma once

#include "misc.hpp"
#include "pkt_buffer.hpp"
#include "xor_fixed_redundancy.hpp"

template<class super>
class xor_adaptive_redundancy : public xor_fixed_redundancy<super>
{
    typedef xor_fixed_redundancy<super> fixed;

    bool use_status_packets = true;

public:
    xor_adaptive_redundancy(T_arg_list args = {})
    : fixed(args)
    {
        parse(args, false);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            fixed::parse(args);
        }
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        return fixed::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(fixed, buf);

        uint8_t rate = calc_redundancy_rate(buf);
        fixed::parse({ {"xor_fixed_redundancy::amount", rate} }, false);
#ifdef VERBOSE
        printf("Setting a new XOR rate: %u\n", rate);
#endif
        return buf->len;
    }

private:
    uint8_t calc_redundancy_rate(p_pkt_buffer& buf)
    {
        double channel = 1;
        if (isnan(buf->drop_stat))
        {
            channel = fixed::get_channel_success_rate();
#ifdef VERBOSE
            printf("Channel success rate (from fixed) %f\n", channel);
#endif
        }
        else
        {
            channel = buf->drop_stat;
#ifdef VERBOSE
            printf("Channel success rate (from buf) %f\n", channel);
#endif
        }

        if (channel < 0.8)
        {
            // 100 % redundancy
            return 1;
        }
        else if (channel < 0.925)
        {
            // 50 % redundancy
            return 2;
        }
        else if (channel < 0.945)
        {
            // 33 % redundancy
            return 3;
        }
        else if (channel < 0.96)
        {
            // 25 % redundancy
            return 4;
        }
        else if (channel < 0.975)
        {
            // 20 % redundancy
            return 5;
        }
        else if (channel < 0.99)
        {
            // 17 % redundancy
            return 6;
        }
        // 0 % redundancy
        return 0;
    }
};
