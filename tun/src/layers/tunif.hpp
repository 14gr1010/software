#pragma once

#include <cstdlib> // exit()
#include <cstring> // Strings
#include <cstdio> // perror()
#include <net/if.h> // IFNAMSIZ + ifreq
#include <sys/ioctl.h> // ioctl()
#include <fcntl.h> // open() + O_RDWR
#include <unistd.h> // close()
#include <linux/if_tun.h> // IFF_TUN + IFF_NO_PI + TUNSETIFF

#include "misc.hpp"
#include "pkt_buffer.hpp"

template<class super>
class tunif : public super
{
    char if_name[IFNAMSIZ];
    int tun_fd = -1;

public:
    tunif(T_arg_list args = {})
    : super(args)
    {
        parse(args, false);
        create_tun_fd();
    }

    ~tunif()
    {
#ifdef VERBOSE
        printf("Closing TUN interface (%s) fd=%i\n", if_name, tun_fd);
#endif
        close(tun_fd);
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        strncpy(if_name, get_arg("tunif::name", "", args).c, IFNAMSIZ);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(tun_fd, 0);

        return super::read_pkt(buf);
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        ASSERT_GT(tun_fd, 0);

        return super::write_pkt(buf);
    }

    int fd() const
    {
        return tun_fd;
    }

    char* get_if_name() const
    {
        return if_name;
    }

private:
    int create_tun_fd()
    {
        ASSERT_EQ(tun_fd, -1);

        size_t if_name_len = strnlen(if_name, IFNAMSIZ+1);
        ASSERT_GT(if_name_len, 0);
        ASSERT_LTE(if_name_len, IFNAMSIZ);

        int flags = IFF_TUN | IFF_NO_PI;
        ifreq ifr;
        int tmp_fd;
        int err;
        char clonedev[100];

        strncpy(clonedev, "/dev/net/tun", sizeof(clonedev));

        if ((tmp_fd = open(clonedev , O_RDWR)) < 0 )
        {
            perror("Opening /dev/net/tun");
            exit(1);
        }

        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = flags;

        strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

        if ((err = ioctl(tmp_fd, TUNSETIFF, (void *)&ifr)) < 0)
        {
            char errstr[100];
            sprintf(errstr, "ioctl(TUNSETIFF) on fd=%i and if=%s",
                    tmp_fd, ifr.ifr_name);
            perror(errstr);
            close(tmp_fd);
            exit(1);
        }

#ifdef VERBOSE
        printf("Created TUN interface fd=%i\n", tmp_fd);
#endif
        tun_fd = tmp_fd;
        return 0;
    }
};
