#include <cstring>

#include "pc.hpp"
#include "quit.hpp"
#include "layers/kodo_on_the_fly_redundancy.hpp"

#include "../help_strings.hpp"

int main(int argc, char** argv)
{
    char tun[6];
    char local_ip[16];
    char local_port[] = "55555";
    char remote_ip[16];
    char remote_port[] = "55555";

    if (argc < 6 || argc > 8)
    {
        std::cout << strrchr(argv[0], '/')+1 << " [tunif] [kodo redundancy factor] [decode stat file] [local ip] [remote ip] [local port = 55555] [remote port = 55555]" << std::endl;
        std::cout << "Listening for 'print_decode_stat' on 127.0.0.1:54321 (UDP)" << std::endl;

        std::cout << std::endl
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

    connector<kodo_on_the_fly_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

    c.udp->parse({ {"drop_stats::file_name", "/dev/null"} });

    double redundancy = atoi(argv[2])/100.0;
    c.tun->parse({ {"kodo::tx_redundancy", redundancy} });

    c.open_stat_file(argv[3]);

    wait_for_quit();

    return 0;
}
