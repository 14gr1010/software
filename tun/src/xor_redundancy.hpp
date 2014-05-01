#pragma once

#include <stdint.h> // uint8_t

#include "pkt_buffer.hpp"

typedef uint16_t T_pkt_len;

enum T_pkt_redundancy
{
    SYSTEMATIC_PKT = 1,
    REDUNDANCY_PKT = 2,
} __attribute__ ((__packed__));

struct T_pkt_id_len
{
    T_pkt_id id;
    T_pkt_len len;
};

static auto set_packet_xors = set_packet_field<uint8_t>;
static auto get_packet_xors = get_packet_field<uint8_t>;

static auto set_packet_type = set_packet_field<T_pkt_redundancy>;
static auto get_packet_type = get_packet_field<T_pkt_redundancy>;

static auto set_packet_id = set_packet_field<T_pkt_id>;
static auto get_packet_id = get_packet_field<T_pkt_id>;

static auto set_packet_len = set_packet_field<T_pkt_len>;
static auto get_packet_len = get_packet_field<T_pkt_len>;
