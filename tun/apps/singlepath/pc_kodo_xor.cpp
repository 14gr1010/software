#include <cstring>

#include "len_split.hpp"
#include "quit.hpp"

int main(int argc, char** argv)
{
    char tun[6];
    char local_ip[16];
    char local_port[] = "55555";
    char remote_ip[16];
    char remote_port[] = "55555";

    if (argc < 7 || argc > 9)
    {
        std::cout << strrchr(argv[0], '/')+1 << " [tunif] [kodo redundancy factor] [XOR amount] [decode stat file] [local ip] [remote ip] [local port = 55555] [remote port = 55555]" << std::endl;
        std::cout << "Listening for 'print_decode_stat' on 127.0.0.1:54321 (UDP)" << std::endl;
        return 1;
    }

    if (argc > 7)
    {
        strncpy(local_port, argv[7], 5);
    }

    if (argc > 8)
    {
        strncpy(remote_port, argv[8], 5);
    }

    strncpy(tun, argv[1], 5);
    strncpy(local_ip, argv[5], 15);
    strncpy(remote_ip, argv[6], 15);

    split_connector c(tun, local_ip, local_port, remote_ip, remote_port);

    c.udp->parse({ {"drop_stats::file_name", "/dev/null"} });

    double redundancy = atoi(argv[2])/100.0;
    c.K->parse({ {"kodo::tx_redundancy", redundancy} });

    int xor_amount = atoi(argv[3]);
    c.X->parse({ {"xor_fixed_redundancy::amount", xor_amount} });

    c.open_stat_file(argv[4]);

    wait_for_quit();

    return 0;
}
