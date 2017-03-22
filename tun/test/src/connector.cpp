#include "connector.hpp"

#include "../test_case.hpp"

#define NO_PRINT

#include <time.h>

#include "pc.hpp"
#include "pc_multipath.hpp"
//#include "pc_recoder.hpp"
#include "pc_forwarder.hpp"
#include "layers/no_redundancy.hpp"
#include "layers/xor_fixed_redundancy.hpp"
#include "layers/xor_adaptive_redundancy.hpp"
//#include "layers/kodo_fixed_redundancy.hpp"
//#include "layers/kodo_on_the_fly_redundancy.hpp"
//#include "layers/kodo_fixed_simple_redundancy.hpp"
//#include "layers/kodo_on_the_fly_simple_redundancy.hpp"
#include "layers/final_layer.hpp"

void create_connector_class_1()
{
    INIT_TEST_CASE;

    const char tun[] = "tun0";
    const char local_ip[] = "127.0.0.1";
    const char local_port[] = "55555";
    const char remote_ip[] = "127.0.0.1";
    const char remote_port[] = "55556";

    connector<no_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

    const timespec T = {0, 100000};
    int r = nanosleep(&T, NULL);
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_class_2()
{
    INIT_TEST_CASE;

    const char tun[] = "tun0";
    const char local_ip[] = "127.0.0.1";
    const char local_port[] = "55555";
    const char remote_ip[] = "127.0.0.1";
    const char remote_port[] = "55556";

    connector<xor_fixed_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

    const timespec T = {0, 100000};
    int r = nanosleep(&T, NULL);
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_class_3()
{
    INIT_TEST_CASE;

    const char tun[] = "tun0";
    const char local_ip[] = "127.0.0.1";
    const char local_port[] = "55555";
    const char remote_ip[] = "127.0.0.1";
    const char remote_port[] = "55556";

    connector<xor_adaptive_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

    const timespec T = {0, 100000};
    int r = nanosleep(&T, NULL);
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_class_4()
{
    INIT_TEST_CASE;

//    const char tun[] = "tun0";
//    const char local_ip[] = "127.0.0.1";
//    const char local_port[] = "55555";
//    const char remote_ip[] = "127.0.0.1";
//    const char remote_port[] = "55556";

//    connector<kodo_fixed_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

//    const timespec T = {0, 100000};
//    int r = nanosleep(&T, NULL);
//    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_class_5()
{
    INIT_TEST_CASE;

//    const char tun[] = "tun0";
//    const char local_ip[] = "127.0.0.1";
//    const char local_port[] = "55555";
//    const char remote_ip[] = "127.0.0.1";
//    const char remote_port[] = "55556";

//    connector<kodo_on_the_fly_simple_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

//    const timespec T = {0, 100000};
//    int r = nanosleep(&T, NULL);
//    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_class_6()
{
    INIT_TEST_CASE;

//    const char tun[] = "tun0";
//    const char local_ip[] = "127.0.0.1";
//    const char local_port[] = "55555";
//    const char remote_ip[] = "127.0.0.1";
//    const char remote_port[] = "55556";

//    connector<kodo_fixed_simple_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

//    const timespec T = {0, 100000};
//    int r = nanosleep(&T, NULL);
//    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_class_7()
{
    INIT_TEST_CASE;

//    const char tun[] = "tun0";
//    const char local_ip[] = "127.0.0.1";
//    const char local_port[] = "55555";
//    const char remote_ip[] = "127.0.0.1";
//    const char remote_port[] = "55556";

//    connector<kodo_on_the_fly_redundancy> c(tun, local_ip, local_port, remote_ip, remote_port);

//    const timespec T = {0, 100000};
//    int r = nanosleep(&T, NULL);
//    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_recoder_class_1()
{
    INIT_TEST_CASE;

//    const char local_ip_1[] = "127.0.0.1";
//    const char local_port_1[] = "55555";
//    const char remote_ip_1[] = "127.0.0.1";
//    const char remote_port_1[] = "55556";
//    const char local_ip_2[] = "127.0.0.1";
//    const char local_port_2[] = "55557";
//    const char remote_ip_2[] = "127.0.0.1";
//    const char remote_port_2[] = "55558";

//    recoder c(local_ip_1, local_port_1, remote_ip_1, remote_port_1,
//              local_ip_2, local_port_2, remote_ip_2, remote_port_2);

//    const timespec T = {0, 100000};
//    int r = nanosleep(&T, NULL);
//    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_forwarder_class_1()
{
    INIT_TEST_CASE;

    const char local_ip_1[] = "127.0.0.1";
    const char local_port_1[] = "55555";
    const char remote_ip_1[] = "127.0.0.1";
    const char remote_port_1[] = "55556";
    const char local_ip_2[] = "127.0.0.1";
    const char local_port_2[] = "55557";
    const char remote_ip_2[] = "127.0.0.1";
    const char remote_port_2[] = "55558";

    forwarder c(local_ip_1, local_port_1, remote_ip_1, remote_port_1,
                local_ip_2, local_port_2, remote_ip_2, remote_port_2);

    const timespec T = {0, 100000};
    int r = nanosleep(&T, NULL);
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void create_connector_multipath_class_1()
{
    INIT_TEST_CASE;

//    const char tun[] = "tun0";

//    std::deque<T_tcp_ip_connection> connections;
//
//    T_tcp_ip_connection d;
//    strncpy(d.local_ip, "127.0.0.1", 16);
//    strncpy(d.remote_ip, "127.0.0.1", 16);
//    strncpy(d.local_port, "55555", 6);
//    strncpy(d.remote_port, "55556", 6);
//    connections.push_back(d);

//    strncpy(d.local_ip, "127.0.0.1", 16);
//    strncpy(d.remote_ip, "127.0.0.1", 16);
//    strncpy(d.local_port, "55557", 6);
//    strncpy(d.remote_port, "55558", 6);
//    connections.push_back(d);

//    connector_multipath<kodo_on_the_fly_redundancy> c(tun, connections);

//    const timespec T = {0, 100000};
//    int r = nanosleep(&T, NULL);
//    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}
