#define NO_PRINT

#ifndef NO_PRINT
#define VERBOSE
//#define PRINT_IO_BUFFER
#endif

#include <thread> // std::thread
#include <fstream> // std::fstream

#include "layers/tunif.hpp"
#include "layers/fd_io.hpp"
#include "layers/split_data_length.hpp"
#include "layers/no_redundancy.hpp"
#include "layers/xor_fixed_redundancy.hpp"
#include "layers/kodo_on_the_fly_redundancy.hpp"
#include "layers/udp_socket.hpp"
#include "layers/socket_io.hpp"
#include "layers/drop_stats.hpp"
#include "layers/udp_socket_unconnected.hpp"
#include "layers/final_layer.hpp"
#include "buffer_pool.hpp"

class split_connector
{
    typedef split_data_length::substack substack;

    typedef xor_fixed_redundancy<
            substack
            > xor_stack;

    typedef kodo_on_the_fly_redundancy<
            substack
            > kodo_stack;

    typedef no_redundancy<
            substack
            > direct_stack;

    typedef tunif<
            fd_io<
            split_data_length
            >> splitter;

    typedef udp_socket<
            socket_io<
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
    splitter* sp;
    udp_stack* udp;

    xor_stack* X;
    kodo_stack* K;
    direct_stack* D;

    split_connector(const char* tunif, const char* local_ip, const char* local_port,
                    const char* remote_ip, const char* remote_port)
    {
        udp = new udp_stack({ {"udp_socket::local_ip", local_ip},
                              {"udp_socket::local_port", local_port},
                              {"udp_socket::remote_ip", remote_ip},
                              {"udp_socket::remote_port", remote_port} });

        sp = new splitter({ {"tunif::name", tunif} });

        X = new xor_stack({ {"xor_fixed_redundancy::amount", 5} });

        K = new kodo_stack({ {"kodo::tx_gen_size", 10},
                             {"kodo::tx_redundancy", 1.167},
                             {"kodo::rx_pkt_timeout", 0.001} });

        D = new direct_stack();

        external_input = new external_udp();

        sp->set_stack(X, 100);
//        sp->set_stack(D, 0);
        sp->set_stack(K, 2000);

        udp->connect_signal_fw(boost::bind(&splitter::write_pkt, sp, _1));
        X->connect_signal_fw(boost::bind(&udp_stack::write_pkt, udp, _1));
        K->connect_signal_fw(boost::bind(&udp_stack::write_pkt, udp, _1));
        D->connect_signal_fw(boost::bind(&udp_stack::write_pkt, udp, _1));
        start_thread();
    }

    ~split_connector()
    {
        stop_thread();
        delete sp;
        delete udp;
        delete X;
        delete K;
        delete D;
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
        thread_handle = std::thread(&split_connector::run, this);
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
        size_t timeouts = 0;
        while (run_thread)
        {
            int nfds = 0;
            fd_set read_set;
            FD_ZERO(&read_set);

            nfds = std::max(nfds, sp->fd());
            nfds = std::max(nfds, udp->fd());
            nfds = std::max(nfds, external_input->fd());

            FD_SET(sp->fd(), &read_set);
            FD_SET(udp->fd(), &read_set);
            FD_SET(external_input->fd(), &read_set);

            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 1;
            int retval = select(nfds+1, &read_set, NULL, NULL, &tv);
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
                timeouts++;
                continue;
            }

            write_pkts();

            if (FD_ISSET(sp->fd(), &read_set))
            {
                p_pkt_buffer buf = pool.malloc_buf();
                sp->read_pkt(buf);
                pool.free_buf(buf);
            }
            if (FD_ISSET(udp->fd(), &read_set))
            {
                p_pkt_buffer buf = pool.malloc_buf();
                udp->read_pkt(buf);
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
                }
                else if (strcmp("print_decode_stat", (char*)buf->head) == 0)
                {
                    K->print_decode_stat(fio);
                    X->print_decode_stat(fio);
                    fio << std::endl;
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
        while (sp->write_pkt(buf))
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
