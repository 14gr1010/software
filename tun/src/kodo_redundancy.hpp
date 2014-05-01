#pragma once

#include <stdint.h> // uint32_t

#include "pkt_buffer.hpp"

typedef uint16_t T_zeropad;

static auto set_packet_symbol_size = set_packet_field<uint32_t>;
static auto get_packet_symbol_size = get_packet_field<uint32_t>;

static auto set_packet_gen_size = set_packet_field<uint32_t>;
static auto get_packet_gen_size = get_packet_field<uint32_t>;

static auto set_packet_gen = set_packet_field<uint32_t>;
static auto get_packet_gen = get_packet_field<uint32_t>;

static auto set_packet_zeros = set_packet_field<T_zeropad>;
static auto get_packet_zeros = get_packet_field<T_zeropad>;
