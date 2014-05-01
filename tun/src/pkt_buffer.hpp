#pragma once

#include <cstdlib>  // size_t
#include <vector>   // std::vector
#include <cstdio>   // printf() + sprintf()
#include <cstring>  // memcpy() + strlen() + strcpy()
#include <memory>   // std::shared_ptr
#include <stdint.h> // uint8_t
#include <cmath>    // nan()
#include <deque>    // std::deque

#include "misc.hpp"

class pkt_buffer;
typedef std::shared_ptr<pkt_buffer> p_pkt_buffer;
typedef std::deque<p_pkt_buffer> T_pkt_queue;

typedef uint32_t T_pkt_id;
typedef uint16_t T_data_len;

class pkt_buffer
{
    std::vector<uint8_t> buffer;

    pkt_buffer& operator=(const pkt_buffer&) = delete;
    pkt_buffer(const pkt_buffer&) = delete;

public:
    const size_t default_head_room = 100;
    const size_t default_data_size = 1600;
    const size_t default_alignment = 16;

    uint8_t *data;
    uint8_t *head;
    uint8_t *head_min;
    size_t len;
    T_pkt_id id;
    uint64_t time_stamp;
    uint8_t protocol;
    double drop_stat;
    T_data_len data_len;

    pkt_buffer()
    {
        buffer.resize(default_data_size + default_head_room + default_alignment);
        clear();
    }

    pkt_buffer(p_pkt_buffer old)
    {
        buffer = old->buffer;
        data = old->data - &old->buffer[0] + &buffer[0];
        head = old->head - &old->buffer[0] + &buffer[0];
        head_min = old->head_min - &old->buffer[0] + &buffer[0];
        len = old->len;
        data_len = old->data_len;
        time_stamp = old->time_stamp;
        id = old->id;
        drop_stat = old->drop_stat;
    }

    void clear()
    {
        data = &buffer[0] + default_head_room + default_alignment;
        align_data();
        head = data;
        head_min = head;
        len = 0;
        data_len = 0;
        time_stamp = 0;
        id = 0;
        drop_stat = nan("");
    }
    
    size_t get_max_len() const
    {
        return buffer.size();
    }

    bool is_empty() const
    {
        return (len == 0);
    }

    void head_push(size_t amount)
    {
        head -= amount;
        len += amount;
        head_min = std::min(head_min , head);

        ASSERT_GTE(data, head);
    }

    void head_pull(size_t amount)
    {
        head += amount;
        len -= amount;

        ASSERT_GTE(data, head);
    }

    void data_push(size_t amount)
    {
        len += amount;

        ASSERT_GTE(data, head);
    }

    void data_pull(size_t amount)
    {
        len -= amount;

        ASSERT_GTE(data, head);
    }

    void push(size_t amount)
    {
        head -= amount;
        data -= amount;
        len += amount;
        head_min = std::min(head_min , head);

        ASSERT_GTE(data, head);
    }

    void pull(size_t amount)
    {
        head += amount;
        data += amount;
        len -= amount;

        ASSERT_GTE(data, head);
    }

    size_t get_head_room() const
    {
        return head - &buffer[0];
    }

    size_t zero_pad_to(size_t new_size)
    {
        ASSERT_GTE(new_size, len);

        if (new_size == len)
        {
            return 0;
        }

        size_t current_headroom = get_head_room();
        ASSERT_LT(current_headroom + new_size, buffer.size());

        size_t zeros = new_size-len;
        memset(head+len, 0, zeros);

        len = new_size;

        return zeros;
    }

    void xor_in(p_pkt_buffer& pkt)
    {
        size_t xor_len = std::max(len, pkt->len);
        size_t xor_len_min = std::min(len, pkt->len);

        size_t current_headroom = get_head_room();
        if (buffer.size() < current_headroom + xor_len)
        {
            size_t current_dataroom = data - &buffer[0];
            size_t head_room = std::max(default_head_room, current_headroom);
            buffer.resize(head_room + xor_len);
            head = &buffer[0] + current_headroom;
            data = &buffer[0] + current_dataroom;
        }

        xor_pkts(pkt, xor_len_min);

        if (len > xor_len_min)
        {
            // 'this' buffer is the largest, i.e., no more XOR is needed
            return;
        }

        // No need to XOR with zeros, memcpy will do the same
        memcpy(head + xor_len_min, pkt->head + xor_len_min, xor_len - xor_len_min);

        // Adjust len to reflect the length after XOR
        len = xor_len;
    }

    void xor_out(p_pkt_buffer& pkt, size_t xor_len)
    {
        size_t current_headroom = get_head_room();
        ASSERT_GTE(buffer.size(), current_headroom + xor_len);

        xor_pkts(pkt, xor_len);

        len = xor_len;
    }

    void print_head(size_t limit, const char* msg = NULL) const
    {
        print_buffer(head, limit, msg);
    }

    void print_head_min(size_t limit, const char* msg = NULL) const
    {
        print_buffer(head_min, limit, msg);
    }

private:
    void print_buffer(uint8_t* start, size_t limit, const char* msg) const
    {
        char str[10000];
        str[0] = 0;
        if (NULL != msg)
        {
            strcpy(str, msg);
            sprintf(str+strlen(str), " (%lu bytes): ", len);
        }
        size_t l = strlen(str);
        for (size_t n = 0; n < limit; n++)
        {
            uint8_t X = start[n];
            l += snprintf(str+l, 4, "%02X ", X);
        }
        str[l-1] = 0;
        printf("%s\n", str);
    }

    void xor_pkts(p_pkt_buffer& pkt, size_t xor_len)
    {
        // For mysterious reasons g++ won't vectorize if the pointer is 'uint8_t*'
        char *fast_array1 = (char*)head;
        char *fast_array2 = (char*)pkt->head;
        for (size_t i = 0; i < xor_len; i++)
        {
            fast_array1[i] ^= fast_array2[i];
        }
    }

    void align_data()
    {
        data = (uint8_t*)(((uintptr_t)data + default_alignment - 1) & ~(default_alignment - 1));
    };
};

template<class T>
void set_packet_field(p_pkt_buffer& buf, T new_amount)
{
    ASSERT_GTE(buf->get_head_room(), sizeof(T));

    buf->head_push(sizeof(T));
    *(T*)buf->head = new_amount;
}

template<class T>
T get_packet_field(p_pkt_buffer& buf)
{
    T amount = *(T*)buf->head;
    if (buf->head == buf->data)
    {
        buf->pull(sizeof(T));
    }
    else
    {
        buf->head_pull(sizeof(T));
    }
    return amount;
}

template<class T>
T read_packet_field(p_pkt_buffer& buf)
{
    return *(T*)buf->head;
}

inline p_pkt_buffer new_pkt_buffer()
{
    return std::make_shared<pkt_buffer>();
}

inline p_pkt_buffer copy_pkt_buffer(p_pkt_buffer buf)
{
    return std::make_shared<pkt_buffer>(buf);
}
