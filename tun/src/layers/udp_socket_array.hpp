#pragma once

#include <netdb.h> // IPPROTO_UDP
#include <cstring> // memset
#include <errno.h> // errno
#include <deque> // std::deque

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class socket_switch, class super>
class udp_socket_array : public super, public socket_switch
{
    struct T_socket_info
    {
        int socket_fd = -1;
        char constr[100];
        char local_ip[20];
        char local_port[20];
        char remote_ip[20];
        char remote_port[20];
    };

    std::deque<T_socket_info> sockets;
    int socket_fd_r = -1;

public:
    udp_socket_array(T_arg_list args = {})
        : super(args)
    {
        parse(args, false);
    }

    ~udp_socket_array()
    {
        for (auto i : sockets)
        {
#ifdef VERBOSE
            printf("Closing UDP socket (%s) fd=%i\n", i.constr, i.socket_fd);
#endif
            close(i.socket_fd);
        }
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        socket_fd_r = get_arg("udp_socket_array::fd_r", socket_fd_r, args).i;
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(sockets.size(), 0);

        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(sockets.size(), 0);

        return super::write_pkt(buf);
    }

    size_t get_num_of_sockets()
    {
        return sockets.size();
    }

    size_t get_generator_max()
    {
        return sockets.size();
    }

    int fd_r()
    {
        return socket_fd_r;
    }

    int fd_w()
    {
        return sockets[socket_switch::get_index()].socket_fd;
    }

    int loop_fd()
    {
        static size_t i = -1;
        ++i;
        if (i < sockets.size())
        {
            return sockets[i].socket_fd;
        }
        i = -1;
        return -1;
    }

    void create_socket(const char* local_ip_string,
                       const char* local_port_string,
                       const char* remote_ip_string,
                       const char* remote_port_string)
    {
        T_socket_info tmp;
        strncpy(tmp.local_ip, local_ip_string, 20);
        strncpy(tmp.local_port, local_port_string, 20);
        strncpy(tmp.remote_ip, remote_ip_string, 20);
        strncpy(tmp.remote_port, remote_port_string, 20);

        strcpy(tmp.constr, local_ip_string);
        strcat(tmp.constr, ":");
        strcat(tmp.constr, local_port_string);
        strcat(tmp.constr, " > ");
        strcat(tmp.constr, remote_ip_string);
        strcat(tmp.constr, ":");
        strcat(tmp.constr, remote_port_string);

        addrinfo local_readable_server_info;
        addrinfo* local_server_info;

        //Reset the struct hints
        memset(&local_readable_server_info, 0, sizeof(addrinfo));
        //Set to ipv4
        local_readable_server_info.ai_family = AF_INET;
        //Set type to  datagram (Unreliable)
        local_readable_server_info.ai_socktype = SOCK_DGRAM;
        //Ensure that udp is used
        local_readable_server_info.ai_protocol = IPPROTO_UDP;

        int s = getaddrinfo(local_ip_string, local_port_string, &local_readable_server_info, &local_server_info);
        if (s != 0)
        {
            int err = errno;
            char errstr[300];
            sprintf(errstr, "ERROR: While executing: getaddrinfo. Return code: %i\n", s);
            if (s == EAI_SYSTEM)
            {
                strcat(errstr, strerror(err));
            }
            printf("%s", errstr);
            exit(1);
        }

        addrinfo remote_readable_server_info;
        addrinfo* remote_server_info;

        //Reset the struct hints
        memset(&remote_readable_server_info, 0, sizeof(addrinfo));
        //Set to ipv4
        remote_readable_server_info.ai_family = AF_INET;
        //Set type to  datagram (Unreliable)
        remote_readable_server_info.ai_socktype = SOCK_DGRAM;
        //Ensure that udp is used
        remote_readable_server_info.ai_protocol = IPPROTO_UDP;

        s = getaddrinfo(remote_ip_string, remote_port_string, &remote_readable_server_info, &remote_server_info);
        if (s != 0)
        {
            int err = errno;
            char errstr[300];
            sprintf(errstr, "ERROR: While executing: getaddrinfo. Return code: %i\n", s);
            if (s == EAI_SYSTEM)
            {
                strcat(errstr, strerror(err));
            }
            printf("%s", errstr);
            exit(1);
        }

        int tmp_fd = socket(local_server_info->ai_family,
                            local_server_info->ai_socktype,
                            local_server_info->ai_protocol);
        if (tmp_fd < 0)
        {
            perror("ERROR: While getting socket");
            exit(1);
        }

        int ONE = 1;
        if (setsockopt(tmp_fd, SOL_SOCKET, SO_REUSEADDR, &ONE, sizeof(ONE)))
        {
            perror("Setting socket option 'SO_REUSEADDR' failed");
            exit(1);
        }

        if (bind(tmp_fd, local_server_info->ai_addr, local_server_info->ai_addrlen))
        {
            perror("Binding socket failed");
            exit(1);
        }

        if (connect(tmp_fd, remote_server_info->ai_addr, remote_server_info->ai_addrlen))
        {
            perror("Connect socket failed");
            exit(1);
        }

#ifdef VERBOSE
        printf("Created UDP socket with fd=%i\n", tmp_fd);
#endif
        freeaddrinfo(local_server_info);
        freeaddrinfo(remote_server_info);

        tmp.socket_fd = tmp_fd;
        sockets.push_back(tmp);
    }
};
