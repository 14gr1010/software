#include <cstring>

#include "pc_multipath.hpp"
#include "quit.hpp"
#include "layers/kodo_on_the_fly_redundancy.hpp"

#include "../help_strings.hpp"

int main(int argc, char** argv)
{
    char tun[6];

    if (argc < 12 || (argc % 4) != 0)
    {
        std::cout << strrchr(argv[0], '/')+1 << " [tunif] [kodo redundancy factor] [decode stat file] [local ip 1] [remote ip 1] [local port 1] [remote port 1] [local ip 2] [remote ip 2] [local port 2] [remote port 2] ..." << std::endl;
        std::cout << "Listening for 'print_decode_stat' on 127.0.0.1:54321 (UDP)" << std::endl;

        std::cout << std::endl
                  << "The tunnel has X (X >= 2) sets of options, one for each node it is connected to" << std::endl
                  << "[tunif]                     " TUNIF_HELP_STRING << std::endl
                  << "[kodo redundancy factor]    " KODO_REDUNDANCY_FACTOR_HELP_STRING << std::endl
                  << "[decode stat file]          " DECODE_STAT_FILE_HELP_STRING << std::endl
                  << "[local ip]                  " LOCAL_IP_HELP_STRING << std::endl
                  << "[remote ip]                 " REMOTE_IP_HELP_STRING << std::endl
                  << "[local port]                " LOCAL_PORT_HELP_STRING << std::endl
                  << "[remote port]               " REMOTE_PORT_HELP_STRING << std::endl
                  << std::endl;

        return 1;
    }

    strncpy(tun, argv[1], 6);

    std::deque<T_tcp_ip_connection> connections;
    for (int i = 4; i < argc; i += 4)
    {
        T_tcp_ip_connection d;
        strncpy(d.local_ip, argv[i+0], 16);
        strncpy(d.remote_ip, argv[i+1], 16);
        strncpy(d.local_port, argv[i+2], 6);
        strncpy(d.remote_port, argv[i+3], 6);
        connections.push_back(d);
    }
    
    int N = connections.size();

    connector_multipath<kodo_on_the_fly_redundancy> c(tun, connections);

    for (int i = 0; i < N-1; ++i)
    {
        c.udp->set_distribution(0, 100/N);
    }
    c.udp->set_distribution(N-1, (100/N+100%N));

    double redundancy = atoi(argv[2])/100.0;
    c.tun->parse({ {"kodo::tx_redundancy", redundancy} });

    c.open_stat_file(argv[3]);

    wait_for_quit();

    return 0;
}
