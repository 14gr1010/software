#pragma once

#include <sys/socket.h> // sockaddr
#include <stdint.h> // uint32_t
#include <cstdio> // perror()

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class socket_io : public super
{
    virtual int fd() = 0;

    bool print_stats = false;
    int read_pkts = 0;
    int write_pkts = 0;
    uint64_t last_stat_print = time_stamp();

public:
    socket_io(T_arg_list args = {})
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

        print_stats = get_arg("socket_io::print_stats", {print_stats}, args).b;
    }


    virtual int read_pkt(p_pkt_buffer& buf)
    {
        size_t len = buf->len;
        if (0 == len)
        {
            len = buf->default_data_size;
        }

        if (print_stats)
        {
            uint64_t now = time_stamp();
            ++read_pkts;
            if (now > last_stat_print + 1000000)
            {
                last_stat_print = now;
                print_stats_out();
            }
        }

        int n = recv(fd(), buf->head, len, 0);
        if (n > 0)
        {
            buf->data_push(n);
#ifdef VERBOSE
            printf("Received %lu bytes through socket fd=%i\n", buf->len, fd());
#endif
        }
        else
        {
            buf->clear();
            close(fd());
            char errstr[100];
            sprintf(errstr, "Reading max %lu bytes from socket fd=%i return code=%i", len, fd(), n);
            perror(errstr);
            assert(0);
        }

#ifdef PRINT_IO_BUFFER
        buf->print_head(PRINT_IO_BUFFER_SIZE, "Read buffer");
#endif
        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);        

        return write_pkt_to_socket(buf);
    }

private:
    void print_stats_out()
    {
        printf("Read: %i\t Write: %i\n", read_pkts, write_pkts);
    }

    int write_pkt_to_socket(p_pkt_buffer& buf)
    {
#ifdef VERBOSE
        printf("Send %lu bytes through socket fd=%i\n", buf->len, fd());
#endif

        if (print_stats)
        {
            uint64_t now = time_stamp();
            ++write_pkts;
            if (now > last_stat_print + 1000000)
            {
                last_stat_print = now;
                print_stats_out();
            }
        }

        int n = send(fd(), buf->head, buf->len, 0);
        if (n <  0)
        {
            buf->print_head(buf->len, "Buffer");
            close(fd());
            char errstr[100];
            sprintf(errstr, "Writing %lu bytes to socket fd=%i return code=%i", buf->len, fd(), n);
            perror(errstr);
            assert(0);
        }

#ifdef PRINT_IO_BUFFER
        buf->print_head(PRINT_IO_BUFFER_SIZE, "Write buffer");
#endif
        buf->clear();
        
        return n;
    }

};
