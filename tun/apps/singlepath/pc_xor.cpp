#include <cstring>

#include "pc.hpp"
#include "quit.hpp"

#include "layers/xor_fixed_redundancy.hpp"

int main(int argc, char** argv)
{
    char tun[6];
    char local_ip[16];
    char local_port[] = "55555";
    char remote_ip[16];
    char remote_port[] = "55555";

    if (argc < 6 || argc > 8)
    {
        std::cout << strrchr(argv[0], '/')+1 << " [tunif] [XOR amount] [decode stat file] [local ip] [remote ip] [local port = 55555] [remote port = 55555]" << std::endl;
        std::cout << "Listening for 'print_decode_stat' on 127.0.0.1:54321 (UDP)" << std::endl;
        return 1;
    }

    if (argc > 6)
    {
        strncpy(local_port, argv[6], 6);
    }

    if (argc > 7)
    {
        strncpy(remote_port, argv[7], 6);
    }

    strncpy(tun, argv[1], 6);
    strncpy(local_ip, argv[4], 16);
    strncpy(remote_ip, argv[5], 16);

    connector<xor_fixed_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

    c.udp->parse({ {"drop_stats::file_name", "/dev/null"} });

    int xor_amount = atoi(argv[2]);
    c.tun->parse({ {"xor_fixed_redundancy::amount", xor_amount} });

    c.open_stat_file(argv[3]);

    wait_for_quit();

    return 0;
}
