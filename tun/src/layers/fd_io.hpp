#pragma once

#include <cstdlib> // exit() + size_t
#include <cstdio> // sprintf() + perror()
#include <unistd.h> // read()

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class fd_io : public super
{
    virtual int fd() const = 0;

public:
    fd_io(T_arg_list args = {})
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
        size_t len = buf->len;
        if (0 == len)
        {
            len = buf->default_data_size;
        }
        
        int n = read(fd(), buf->data, len);
        if (n <  0)
        {
            close(fd());
            char errstr[100];
            sprintf(errstr, "Reading %lu bytes from fd: %i return code=%i", len, fd(), n);
            perror(errstr);
//            exit(1);
            assert(0);
        }

#ifdef VERBOSE
        printf("Read %i bytes from fd=%i\n", n, fd());
#endif
        buf->data_push(n);

#ifdef PRINT_IO_BUFFER
        buf->print_head(PRINT_IO_BUFFER_SIZE, "Buffer");
#endif
        return super::read_pkt(buf);
    }
    
    virtual int write_pkt(p_pkt_buffer& buf)
    {
        WRITE(super, buf);
        
        if (0 == buf->len)
        {
            return 0;
        }

#ifdef VERBOSE
        printf("Writing %lu bytes to fd=%i\n", buf->len, fd());
#endif
        ASSERT_EQ(buf->data, buf->head);

        int n = write(fd(), buf->data, buf->len);
        if (n <  0)
        {
            buf->print_head(buf->len, "Buffer");
            close(fd());
            char errstr[100];
            sprintf(errstr, "Writing %lu bytes to fd=%i return code=%i", buf->len, fd(), n);
            super::failure();

            perror(errstr);
            assert(0);
        }
        
#ifdef PRINT_IO_BUFFER
        buf->print_head(PRINT_IO_BUFFER_SIZE, "Buffer");
#endif
        buf->clear();

        return n;
    }
    
};
