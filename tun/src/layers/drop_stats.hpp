#pragma once

#include <fstream> // std::fstream

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class drop_stats : public super
{
    uint32_t read_pkts = 0;
    uint32_t write_pkts = 0;
    uint64_t last_stat_print = time_stamp();
    char file_name[100];
    std::fstream fio;

public:
    drop_stats(T_arg_list args = {})
    : super(args)
    {
        parse(args, false);
    }

    ~drop_stats()
    {
        fio.close();
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }
        strncpy(file_name, get_arg("drop_stats::file_name", "", args).c, 100);
        if (strlen(file_name) > 0)
        {
            ASSERT_EQ(fio.is_open(), false);
            fio.open(file_name, std::fstream::out | std::fstream::app);
            ASSERT_EQ(fio.is_open(), true);
        }
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        uint32_t writes = get_packet_field<uint32_t>(buf);
        buf->drop_stat = (read_pkts+1)/(double)writes;
#ifdef VERBOSE
        printf("Getting 'write_pkts' from buffer with value=%u\n", writes);
#endif
        int n = super::read_pkt(buf);
        if (n > 0)
        {
            ++read_pkts;
        }

        return n;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (!buf->is_empty())
        {
            ++write_pkts;
        }

        int n = super::write_pkt(buf);
        ASSERT_EQ(buf->len, (size_t)n);
        if (n > 0)
        {
#ifdef VERBOSE
            printf("Setting 'write_pkts' into buffer with value=%u\n", write_pkts);
#endif
            set_packet_field<uint32_t>(buf, write_pkts);
        }

        return n;
    }

    void print_drop_stat()
    {
        ASSERT_EQ(fio.is_open(), true);
        fio << read_pkts << "," << write_pkts << std::endl;
    }
};
