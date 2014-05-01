#pragma once

#include "pkt_buffer.hpp"

class buffer_pool
{
    T_pkt_queue inactive;

public:
    buffer_pool() {}
    ~buffer_pool() {}

    p_pkt_buffer malloc_buf()
    {
        if (inactive.size() == 0)
        {
            return new_pkt_buffer();
        }
        p_pkt_buffer next = inactive[0];
        inactive.pop_front();
        return next;
    }

    void free_buf(p_pkt_buffer& buf)
    {
        buf->clear();
        inactive.push_front(buf);
    }
};
