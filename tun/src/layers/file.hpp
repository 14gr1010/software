#pragma once

#include <cstdlib> // exit()
#include <cstring> // Strings
#include <cstdio> // perror()
#include <fcntl.h> // open() + O_RDWR
#include <unistd.h> // close()

#include "misc.hpp"
#include "pkt_buffer.hpp"

#define MAX_FILENAME_LENGTH 1000

template<class super>
class file : public super
{
    char file_name[MAX_FILENAME_LENGTH];
    int file_fd = -1;
    int flags = 0;
    int mode = 0;

public:
    file(T_arg_list args = {})
    : super(args)
    {
        parse(args, false);
        create_file_fd();
    }

    ~file() 
    {
#ifdef VERBOSE
        printf("Closing file fd=%i\n", file_fd);
#endif
        close(file_fd);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        flags = get_arg("file::flags", O_RDWR | O_APPEND | O_CREAT, args).i;
        mode = get_arg("file::mode", 0, args).i;
        strncpy(file_name, get_arg("file::name", "NaP", args).c, MAX_FILENAME_LENGTH);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(file_fd, 0);
        
        return super::write_pkt(buf);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(file_fd, 0);

        return super::read_pkt(buf);
    }

    int fd() const
    {
        return file_fd;
    }

    char* get_file_name() const
    {
        return file_name;
    }

private:
    int create_file_fd()
    {
        ASSERT_EQ(file_fd, -1);

        size_t file_name_len = strnlen(file_name, MAX_FILENAME_LENGTH+1);
        ASSERT_GT(file_name_len, 0);
        ASSERT_LTE(file_name_len, MAX_FILENAME_LENGTH);

        int tmp_fd;
        if ((tmp_fd = open(file_name, flags, mode)) < 0)
        {
            char strerr[MAX_FILENAME_LENGTH+100];
            strcpy(strerr, "Opening file '");
            strcat(strerr, file_name);
            strcat(strerr, "'");
            perror(strerr);
            exit(1);
        }

#ifdef VERBOSE
        printf("Opened file (%s) fd=%i\n", file_name, tmp_fd);
#endif
        file_fd = tmp_fd;
        return 0;
    }
};
