#include <cstring>

#include "pc.hpp"
#include "quit.hpp"
#include "layers/xor_adaptive_redundancy.hpp"

#include "../help_strings.hpp"

int main(int argc, char** argv)
{
    char tun[6];
    char local_ip[16];
    const char local_port[] = "55555";
    char remote_ip[16];
    const char remote_port[] = "55555";

    if (argc != 4)
    {
        std::cout << strrchr(argv[0], '/')+1 << " [tunif] [local ip] [remote ip]" << std::endl;

        std::cout << std::endl
                  << "Uses UDP port 55555 to send and receive, on both nodes" << std::endl
                  << "[tunif]                     " TUNIF_HELP_STRING << std::endl
                  << "[local ip]                  " LOCAL_IP_HELP_STRING << std::endl
                  << "[remote ip]                 " REMOTE_IP_HELP_STRING << std::endl
                  << std::endl;

        return 1;
    }

    strncpy(tun, argv[1], 6);
    strncpy(local_ip, argv[2], 16);
    strncpy(remote_ip, argv[3], 16);

    connector<xor_adaptive_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

    c.udp->parse({ {"drop_stats::file_name", "/dev/null"} });

    wait_for_quit();

    return 0;
}
