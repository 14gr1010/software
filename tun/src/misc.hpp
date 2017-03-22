#pragma once

#include <cassert>  // assert()
#include <iostream> // std::cout()
#include <chrono>   // std::chrono

#include "args.hpp"

#define PRINT_IO_BUFFER_SIZE 40

#define WRITE(x, y)                 \
do {int n = x::write_pkt(y);        \
    if (n <= 0)                     \
    {                               \
        return n;                   \
    }                               \
}                                   \
while (0)

#ifndef NDEBUG
#define __ASSERT__(x, y, z)           \
do {if (!((x) z (y)))                 \
    {                                 \
        std::cout << "Expected:  "    \
                  << #x " " #z " " #y \
                  << "\r\nBut found: "\
                  << x                \
                  << " " #z " "       \
                  << y                \
                  << std::endl;       \
        assert(x z y);                \
    }                                 \
}                                     \
while (0)
#else
#define __ASSERT__(x, y, z) ()
#endif

#define ASSERT_EQ(x,  y) __ASSERT__(x, y, ==)
#define ASSERT_NEQ(x, y) __ASSERT__(x, y, !=)
#define ASSERT_LT(x,  y) __ASSERT__(x, y, <)
#define ASSERT_LTE(x, y) __ASSERT__(x, y, <=)
#define ASSERT_GT(x,  y) __ASSERT__(x, y, >)
#define ASSERT_GTE(x, y) __ASSERT__(x, y, >=)

inline uint64_t time_stamp()
{
    namespace sc = std::chrono;
    static std::chrono::microseconds::rep start = sc::duration_cast<sc::microseconds>(sc::high_resolution_clock::now().time_since_epoch()).count();
    return sc::duration_cast<sc::microseconds>(sc::high_resolution_clock::now().time_since_epoch()).count() - start;
}
