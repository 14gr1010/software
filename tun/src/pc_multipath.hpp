#define NO_PRINT

#ifndef NO_PRINT
#define VERBOSE
#define PRINT_IO_BUFFER
#endif

#include <thread> // std::thread
#include <fstream> // std::fstream

#include "layers/tunif.hpp"
#include "layers/fd_io.hpp"
#include "layers/udp_socket_array.hpp"
#include "layers/socket_array_io.hpp"
#include "layers/drop_stats.hpp"
#include "layers/udp_socket_unconnected.hpp"
#include "layers/socket_io.hpp"
#include "layers/final_layer.hpp"
#include "buffer_pool.hpp"
#include "generators/generator_distribution_random.hpp"

/*
Possible values for 'Redundancy'
    no_redundancy
    xor_fixed_redundancy
    xor_adaptive_redundancy
    kodo_fixed_redundancy
    kodo_on_the_fly_redundancy
    kodo_fixed_simple_redundancy
    kodo_on_the_fly_simple_redundancy
*/

struct T_tcp_ip_connection
{
    char local_ip[16];
    char local_port[6];
    char remote_ip[16];
    char remote_port[6];
};

template <template <class> class Redundancy>
class connector_multipath
{
    typedef tunif<
            fd_io<
            Redundancy<
            final_layer
            >>> tun_stack;

    typedef udp_socket_array<generator_distribution_random,
            socket_array_io<
            drop_stats<
            final_layer
            >>> udp_stack;

    typedef udp_socket_unconnected<
            socket_io<
            final_layer
            >> external_udp;

    buffer_pool pool;
    std::thread thread_handle;
    bool run_thread = false;
    external_udp* external_input;
    std::fstream fio;

public:
    tun_stack* tun;
    udp_stack* udp;

    connector_multipath(const char* tunif,
                        std::deque<T_tcp_ip_connection> connections)
    {
        ASSERT_NEQ(connections.size(), 0);

        udp = new udp_stack({ });

        for (auto i : connections)
        {
            udp->create_socket(i.local_ip, i.local_port,
                               i.remote_ip, i.remote_port);
        }

        tun = new tun_stack({ {"tunif::name", tunif},
                              {"xor_fixed_redundancy::amount", 5},
                              {"kodo::tx_gen_size", 10},
                              {"kodo::tx_redundancy", 1.167},
                              {"kodo::rx_pkt_timeout", 0.001} });

        external_input = new external_udp();

        udp->connect_signal_fw(boost::bind(&tun_stack::write_pkt, tun, _1));
        tun->connect_signal_fw(boost::bind(&udp_stack::write_pkt, udp, _1));

        start_thread();

#ifdef VERBOSE
        printf("%s\n", "connector class created");
#endif
    }

    ~connector_multipath()
    {
        stop_thread();
        delete udp;
        delete tun;
        delete external_input;
        if (fio.is_open())
        {
            fio.close();
        }
    }

    void open_stat_file(char* filename)
    {
        if (strlen(filename) > 0)
        {
            ASSERT_EQ(fio.is_open(), false);
            fio.open(filename, std::fstream::out | std::fstream::app);
            ASSERT_EQ(fio.is_open(), true);
        }
    }

private:
    void start_thread()
    {
#ifdef VERBOSE
        printf("%s", "Starting thread\n");
#endif
        run_thread = true;
        thread_handle = std::thread(&connector_multipath::run, this);
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

            nfds = std::max(nfds, tun->fd());
            int udp_fd = -1;
            while ((udp_fd = udp->loop_fd()) > 0)
            {
                FD_SET(udp_fd, &read_set);
                nfds = std::max(nfds, udp_fd);
            }
            nfds = std::max(nfds, external_input->fd());

            FD_SET(tun->fd(), &read_set);
            FD_SET(external_input->fd(), &read_set);

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

            while ((udp_fd = udp->loop_fd()) > 0)
            {
                if (FD_ISSET(udp_fd, &read_set))
                {
                    udp->parse({ {"udp_socket_array::fd_r", udp_fd} });
                    p_pkt_buffer buf = pool.malloc_buf();
                    udp->read_pkt(buf);
                    pool.free_buf(buf);
                }
            }
            if (FD_ISSET(tun->fd(), &read_set))
            {
                p_pkt_buffer buf = pool.malloc_buf();
                tun->read_pkt(buf);
                pool.free_buf(buf);
            }
            if (FD_ISSET(external_input->fd(), &read_set))
            {
                p_pkt_buffer buf = pool.malloc_buf();
                external_input->read_pkt(buf);
                if (buf->head[buf->len-1] == '\n')
                {
                    buf->head[buf->len-1] = '\0';
                }
                if (strcmp("print_drop_stat", (char*)buf->head) == 0)
                {
                    udp->print_drop_stat();
                    std::cout << "Print complete" << std::endl;
                }
                else if (strcmp("print_decode_stat", (char*)buf->head) == 0)
                {
                    tun->print_decode_stat(fio);
                    fio << std::endl;
                    fio.close();
                    std::cout << "Print complete" << std::endl;
                    exit(0);
                }
                else
                {
                    std::cout << "Unknown command: " << std::string((char*)buf->head, buf->len) << std::endl;
                }
                pool.free_buf(buf);
            }
        }
    }

    void write_pkts()
    {
        p_pkt_buffer buf = pool.malloc_buf();
        while (tun->write_pkt(buf))
        {
            buf->clear();
        }
        while (udp->write_pkt(buf))
        {
            buf->clear();
        }
        pool.free_buf(buf);
    }
};
