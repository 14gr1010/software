#pragma once

#include <cassert>  // assert()
#include <iostream> // std::cout()
#include <boost/date_time/posix_time/posix_time.hpp> // time_stamp

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
    namespace bpt = boost::posix_time;
//    static bpt::ptime start(boost::gregorian::date(1970,1,1));
    static bpt::ptime start = bpt::microsec_clock::local_time();
    bpt::ptime now = bpt::microsec_clock::local_time();
    bpt::time_duration diff = now - start;
    return diff.total_microseconds();
}
