#pragma once

#include <utility> // std::pair

#include "misc.hpp"
#include "pkt_buffer.hpp"

typedef std::pair<p_pkt_buffer, double> pkt_delay_pair;
typedef std::map<T_pkt_id, pkt_delay_pair> pkt_queue;

template<class super>
class delay : public super
{
    // Transmitter
    double tx_delay = 0.001;
    pkt_queue tx_queue;

public:
    delay(T_arg_list args = {})
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

        tx_delay = get_arg("delay::tx", tx_delay, args).d;
        ASSERT_GT(tx_delay, 0);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
#ifdef VERBOSE
        printf("Packet of %lu bytes NOT delayed\n", buf->len);
#endif

        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            if (pkt_ready(tx_queue, buf))
            {
                WRITE(super, buf);
                return buf->len;
            }
        }
        WRITE(super, buf);

#ifdef VERBOSE
        printf("Packet of %lu bytes DELAYED\n", buf->len);
#endif
        store_pkt(tx_queue, buf);

        return 0;
    }

private:
    bool pkt_ready(pkt_queue& queue, p_pkt_buffer& buf)
    {
        if (queue.size() == 0)
        {
            return false;
        }

        uint64_t time_now = time_stamp();
        for (auto i = queue.begin(); i != queue.end(); ++i)
        {
            if (i->second.first->time_stamp + (1000000 * i->second.second) > time_now)
            {
                continue;
            }

            buf->clear();
            buf = i->second.first;
            queue.erase(i);
#ifdef VERBOSE
            printf("Releasing pkt of %lu bytes and id %u\n", buf->len, buf->id);
#endif
            return true;
        }
        return false;
    }

    void store_pkt(pkt_queue& queue, p_pkt_buffer& buf)
    {
#ifdef VERBOSE
        printf("Storing pkt of %lu bytes and id %u\n", buf->len, buf->id);
#endif
        if (queue.count(buf->id) == 0)
        {
            buf->time_stamp = time_stamp();
            queue[buf->id] = pkt_delay_pair(buf, tx_delay);
            buf = new_pkt_buffer();
        }
    }
};
