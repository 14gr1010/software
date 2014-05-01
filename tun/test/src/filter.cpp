#include "filter.hpp"

#include <cstring>

#include "../test_case.hpp"

#include "layers/tcp_filter.hpp"
#include "layers/tcp_udp_filter.hpp"
#include "layers/udp_filter.hpp"
#include "layers/final_layer.hpp"

void filter_1()
{
    INIT_TEST_CASE;

    int r = -1;
    p_pkt_buffer buf = new_pkt_buffer();

    tcp_filter<final_layer> tcp_f;

    iphdr ip_hdr_tcp;
    ip_hdr_tcp.protocol = getprotobyname("tcp")->p_proto;

    iphdr ip_hdr_udp;
    ip_hdr_udp.protocol = getprotobyname("udp")->p_proto;

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memset(buf->data, 0, sizeof(iphdr));
    r = tcp_f.read_pkt(buf);
    TEST_EQ(r, 0);

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memcpy(buf->data, &ip_hdr_tcp, sizeof(iphdr));
    r = tcp_f.read_pkt(buf);
    TEST_EQ(r, sizeof(iphdr));

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memcpy(buf->data, &ip_hdr_udp, sizeof(iphdr));
    r = tcp_f.read_pkt(buf);
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void filter_2()
{
    INIT_TEST_CASE;

    int r = -1;
    p_pkt_buffer buf = new_pkt_buffer();

    udp_filter<final_layer> udp_f;

    iphdr ip_hdr_tcp;
    ip_hdr_tcp.protocol = getprotobyname("tcp")->p_proto;

    iphdr ip_hdr_udp;
    ip_hdr_udp.protocol = getprotobyname("udp")->p_proto;

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memset(buf->data, 0, sizeof(iphdr));
    r = udp_f.read_pkt(buf);
    TEST_EQ(r, 0);

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memcpy(buf->data, &ip_hdr_udp, sizeof(iphdr));
    r = udp_f.read_pkt(buf);
    TEST_EQ(r, sizeof(iphdr));

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memcpy(buf->data, &ip_hdr_tcp, sizeof(iphdr));
    r = udp_f.read_pkt(buf);
    TEST_EQ(r, 0);


    CONCLUDE_TEST_CASE;
}

void filter_3()
{
    INIT_TEST_CASE;

    int r = -1;
    p_pkt_buffer buf = new_pkt_buffer();

    tcp_udp_filter<final_layer> f;

    iphdr ip_hdr_tcp;
    ip_hdr_tcp.protocol = getprotobyname("tcp")->p_proto;

    iphdr ip_hdr_udp;
    ip_hdr_udp.protocol = getprotobyname("udp")->p_proto;

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memset(buf->data, 0, sizeof(iphdr));
    r = f.read_pkt(buf);
    TEST_EQ(r, 0);

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memcpy(buf->data, &ip_hdr_tcp, sizeof(iphdr));
    r = f.read_pkt(buf);
    TEST_EQ(r, sizeof(iphdr));

    buf->clear();
    buf->data_push(sizeof(iphdr));
    memcpy(buf->data, &ip_hdr_udp, sizeof(iphdr));
    r = f.read_pkt(buf);
    TEST_EQ(r, sizeof(iphdr));

    CONCLUDE_TEST_CASE;
}
