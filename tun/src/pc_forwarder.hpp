#define NO_PRINT

#ifndef NO_PRINT
#define VERBOSE
#define PRINT_IO_BUFFER
#endif

#include <thread> // std::thread

#include "layers/udp_socket.hpp"
#include "layers/socket_io.hpp"
#include "layers/final_layer.hpp"
#include "buffer_pool.hpp"

class forwarder
{
    typedef udp_socket<
            socket_io<
            final_layer
            >> udp_forwarder_stack;

    buffer_pool pool;
    std::thread thread_handle;
    bool run_thread = false;

public:
    udp_forwarder_stack* udp1;
    udp_forwarder_stack* udp2;

    forwarder(const char* local_ip_1, const char* local_port_1,
            const char* remote_ip_1, const char* remote_port_1,
            const char* local_ip_2, const char* local_port_2,
            const char* remote_ip_2, const char* remote_port_2)
    {
        udp1 = new udp_forwarder_stack({ {"udp_socket::local_ip", local_ip_1},
                                       {"udp_socket::local_port", local_port_1},
                                       {"udp_socket::remote_ip", remote_ip_1},
                                       {"udp_socket::remote_port", remote_port_1} });

        udp2 = new udp_forwarder_stack({ {"udp_socket::local_ip", local_ip_2},
                                       {"udp_socket::local_port", local_port_2},
                                       {"udp_socket::remote_ip", remote_ip_2},
                                       {"udp_socket::remote_port", remote_port_2} });

        udp1->connect_signal_fw(boost::bind(&udp_forwarder_stack::write_pkt, udp2, _1));
        udp2->connect_signal_fw(boost::bind(&udp_forwarder_stack::write_pkt, udp1, _1));

        start_thread();

#ifdef VERBOSE
        printf("%s\n", "forwarder class created");
#endif
    }

    ~forwarder()
    {
        stop_thread();
        delete udp1;
        delete udp2;
    }

private:
    void start_thread()
    {
#ifdef VERBOSE
        printf("%s", "Starting thread\n");
#endif
        run_thread = true;
        thread_handle = std::thread(&forwarder::run, this);
    }

    void stop_thread()
    {
#ifdef VERBOSE
        printf("%s", "Stopping thread\n");
#endif
        run_thread = false;
        thread_handle.join();
    }

    void run()
    {
        while (run_thread)
        {
            int nfds = 0;
            fd_set read_set;
            FD_ZERO(&read_set);

            nfds = std::max(nfds, udp1->fd());
            nfds = std::max(nfds, udp2->fd());

            FD_SET(udp1->fd(), &read_set);
            FD_SET(udp2->fd(), &read_set);

            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1000;
            int retval = select(nfds+1, &read_set, NULL, NULL, &tv);
            write_pkts();
            if (-1 == retval)
            {
                if (errno == EINTR)
                {
                    // Discard interrupt
                    continue;
                }
                // Error...
                perror("Error while using select");
                exit(1);
            }
            if (0 == retval)
            {
                // Time-out
                continue;
            }

            if (FD_ISSET(udp1->fd(), &read_set))
            {
                p_pkt_buffer buf = pool.malloc_buf();
                udp1->read_pkt(buf);
                pool.free_buf(buf);
            }
            if (FD_ISSET(udp2->fd(), &read_set))
            {
                p_pkt_buffer buf = pool.malloc_buf();
                udp2->read_pkt(buf);
                pool.free_buf(buf);
            }
        }
    }

    void write_pkts()
    {
        p_pkt_buffer buf = pool.malloc_buf();
        while (udp1->write_pkt(buf))
        {
            buf->clear();
        }
        while (udp2->write_pkt(buf))
        {
            buf->clear();
        }
        pool.free_buf(buf);
    }
};
