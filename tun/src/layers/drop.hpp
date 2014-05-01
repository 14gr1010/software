#pragma once

#include <cstdlib> // rand()
#include <cstdio> // printf()

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class drop : public super
{
    double rate = 0;
    unsigned int dropped = 0;
    unsigned int forwarded = 0;

public:
    drop(T_arg_list args = {})
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

        srand(get_arg("drop::seed", 0, args).i);
        rate = get_arg("drop::rate", rate, args).d;
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        if (forward())
        {
            forwarded++;
            return super::read_pkt(buf);
        }

#ifdef VERBOSE
        printf("Dropped (read) packet with %lu bytes\n", buf->len);
#endif
        dropped++;
        buf->clear();

        return 0;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);
        
        if (forward())
        {
#ifdef VERBOSE
            printf("Forwarded (write) packet with %lu bytes\n", buf->len);
#endif
            forwarded++;
            return buf->len;
        }

#ifdef VERBOSE
        printf("Dropped (write) packet with %lu bytes\n", buf->len);
#endif
        dropped++;
        buf->clear();
        
        return 0;
    }

    double get_drop_rate()
    {
        return rate;
    }

    double get_actual_drop_rate()
    {
        unsigned int processed = forwarded + dropped;
        return (forwarded/(double)processed);
    }

    void print_drop_stats()
    {
        unsigned int processed = forwarded + dropped;
        printf("Processed: %u, Forwarded: %u, Dropped: %u, Success rate: %f\n", 
               processed, forwarded, dropped, forwarded/(double)processed);
    }

private:
    bool forward()
    {
        double random = rand()/(double)RAND_MAX;

        return (random >= rate);
    }
};
