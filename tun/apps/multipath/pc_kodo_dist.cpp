#include <cstring>

#include "pc_multipath.hpp"
#include "quit.hpp"
#include "layers/kodo_on_the_fly_redundancy.hpp"

int main(int argc, char** argv)
{
    char tun[6];

    if (argc < 14 || (argc % 5) != 4)
    {
        std::cout << strrchr(argv[0], '/')+1 << " [tunif] [kodo redundancy factor] [decode stat file] [local ip 1] [remote ip 1] [local port 1] [remote port 1] [probability 1] [local ip 2] [remote ip 2] [local port 2] [remote port 2] [probability 1] ..." << std::endl;
        std::cout << "Listening for 'print_decode_stat' on 127.0.0.1:54321 (UDP)" << std::endl;
        return 1;
    }

    strncpy(tun, argv[1], 6);

    std::deque<T_tcp_ip_connection> connections;
    std::vector<int> probabilities;
    probabilities.reserve(argc/5);
    for (int i = 4; i < argc; i += 5)
    {
        T_tcp_ip_connection d;
        strncpy(d.local_ip, argv[i+0], 16);
        strncpy(d.remote_ip, argv[i+1], 16);
        strncpy(d.local_port, argv[i+2], 6);
        strncpy(d.remote_port, argv[i+3], 6);
        connections.push_back(d);
        probabilities.push_back(atoi(argv[i+4]));
    }

    connector_multipath<kodo_on_the_fly_redundancy> c(tun, connections);

    for (size_t i = 0; i < probabilities.size(); ++i)
    {
        c.udp->set_distribution(i, probabilities[i]);
    }

    double redundancy = atoi(argv[2])/100.0;
    c.tun->parse({ {"kodo::tx_redundancy", redundancy} });

    c.open_stat_file(argv[3]);

    wait_for_quit();

    return 0;
}
