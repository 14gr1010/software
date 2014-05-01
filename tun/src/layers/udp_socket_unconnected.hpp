#pragma once

#include <netdb.h> // IPPROTO_UDP
#include <cstring> // memset
#include <errno.h> // errno

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class udp_socket_unconnected : public super
{
    int socket_fd = -1;
    char constr[100];
    char local_ip[20];
    char local_port[20];

public:
    udp_socket_unconnected(T_arg_list args = {})
    : super(args)
    {
        parse(args, false);
        create_socket(local_ip, local_port);
    }
    
    ~udp_socket_unconnected()
    {
#ifdef VERBOSE
        printf("Closing UDP socket (%s) fd=%i\n", constr, socket_fd);
#endif
        close(socket_fd);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        strcpy(local_ip, get_arg("udp_socket_unconnected::ip", "127.0.0.1", args).c);
        strcpy(local_port, get_arg("udp_socket_unconnected::port", "54321", args).c);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(socket_fd, 0);

        return super::read_pkt(buf);
    }
    
    virtual int write_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(socket_fd, 0);
        
        return super::write_pkt(buf);
    }

    int fd()
    {
        return socket_fd;
    }

private:
    void create_socket(const char* local_ip_string, 
                       const char* local_port_string)
    {
        ASSERT_EQ(socket_fd, -1);
    
        strcpy(constr, local_ip_string);
        strcat(constr, ":");
        strcat(constr, local_port_string);
        strcat(constr, " > Unconnected");
            
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
            sprintf(errstr, "ERROR: While executing: getaddrinfo. Return code: %i: %s\n", s, gai_strerror(s));
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

#ifdef VERBOSE
        printf("Created UDP socket with fd=%i\n", tmp_fd);
#endif
        freeaddrinfo(local_server_info);

        socket_fd = tmp_fd;
    }
};
