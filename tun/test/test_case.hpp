#pragma once

extern int __passed_test_cases__;
extern int __failed_test_cases__;
extern int __inconclusive_test_cases__;
extern int __warning_test_cases__;
extern int __max_errors__;

#include <vector>
#include <map>
#include <string>
#include <signal.h>
#include <stdint.h>

#include "misc.hpp"

#define MAX_ERRORS 10 // When a test reaches this amount of errors it will terminate

#define TERMINAL_END "\033[0m"
#define TERMINAL_GREEN "\033[92m"
#define TERMINAL_BLUE "\033[94m"
#define TERMINAL_HEADER "\033[95m"
#define TERMINAL_YELLOW "\033[93m"
#define TERMINAL_RED "\033[91m"

#define TEST_PASSED (0 == __result__)
#define TEST_FAILED (0 != __result__)

#define INIT_TEST_CASE                  \
int __result__ = 0;                     \
int __checks__ = 0;                     \
++__inconclusive_test_cases__;          \
uint64_t __start__ = time_stamp();      \
std::cout << TERMINAL_HEADER            \
          << "INIT: "                   \
          << __FUNCTION__               \
          << std::endl                  \
          << TERMINAL_END;              \
{

#define CONCLUDE_TEST_CASE              \
}                                       \
uint64_t __time__ = ((time_stamp()-     \
                      __start__)/1000); \
--__inconclusive_test_cases__;          \
if (0 == __checks__)                    \
{                                       \
    std::cout << TERMINAL_YELLOW        \
              << "WARNING: No "         \
              << "checks performed"     \
              << "!"                    \
              << TERMINAL_END           \
              << std::endl;             \
    ++__warning_test_cases__;           \
}                                       \
if (0 == __result__)                    \
{                                       \
    ++__passed_test_cases__;            \
    std::cout << TERMINAL_GREEN         \
              << "PASS: '"              \
              << __FUNCTION__           \
              << "'"                    \
              << TERMINAL_END;          \
}                                       \
else                                    \
{                                       \
    ++__failed_test_cases__;            \
    std::cout << TERMINAL_RED           \
              << "FAIL: '"              \
              << __FUNCTION__           \
              << "' with "              \
              << __result__             \
              << " error(s)"            \
              << TERMINAL_END;          \
}                                       \
std::cout << " after "                  \
          << __time__                   \
          << " ms" << std::endl;        \
return;

#define TERMINATE_TEST_CASE             \
do {                                    \
    uint64_t __time__ = ((time_stamp()- \
                      __start__)/1000); \
    --__inconclusive_test_cases__;      \
    if (0 == __checks__)                \
    {                                   \
        std::cout << TERMINAL_YELLOW    \
                  << "WARNING: No "     \
                  << "checks performed" \
                  << "!"                \
                  << TERMINAL_END       \
                  << std::endl;         \
        ++__warning_test_cases__;       \
    }                                   \
    ++__failed_test_cases__;            \
    std::cout << TERMINAL_RED           \
              << "TERMINATED: '"        \
              << __FUNCTION__           \
              << "' after "             \
              << __result__             \
              << " error(s)"            \
              << TERMINAL_END;          \
    std::cout << " after "              \
              << __time__               \
              << " ms" << std::endl;    \
    return;                             \
}                                       \
while (0)

#define __TEST__(x, y, z)               \
do {                                    \
    if (!((x) z (y)))                   \
    {                                   \
        std::cout << TERMINAL_YELLOW    \
                  << "FAIL: "           \
                  << "Expected: "       \
                  << #x " " #z" " #y    \
                  << " But found: "     \
                  << x                  \
                  << " " #z " "         \
                  << y                  \
                  << ", in "            \
                  << __FILE__           \
                  << " on line "        \
                  << __LINE__           \
                  << std::endl          \
                  << TERMINAL_END;      \
        ++__result__;                   \
        if (__result__ >= __max_errors__)\
        {                               \
            TERMINATE_TEST_CASE;        \
        }                               \
    }                                   \
    ++__checks__;                       \
}                                       \
while (0)

#define TEST_EQ(x, y) __TEST__((x), (y), ==)
#define TEST_NEQ(x, y) __TEST__((x), (y), !=)
#define TEST_LT(x, y) __TEST__((x), (y), <)
#define TEST_LTE(x, y) __TEST__((x), (y), <=)
#define TEST_GT(x, y) __TEST__((x), (y), >)
#define TEST_GTE(x, y) __TEST__((x), (y), >=)
