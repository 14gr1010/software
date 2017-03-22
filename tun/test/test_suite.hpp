#pragma once

#include <vector>
#include <map>
#include <string>
#include <signal.h>
#include <stdint.h>
#include <cstring>

#include "misc.hpp"

typedef std::map<std::string, std::vector<void (*)()>> tc_container;

tc_container __tc__;
int __failed_test_cases__ = 0;
int __passed_test_cases__ = 0;
int __inconclusive_test_cases__ = 0;
int __warning_test_cases__ = 0;
int __amount_of_test_cases__ = 0;
int __max_errors__ = 10;
uint64_t __start__ = time_stamp();

#define TERMINAL_END "\033[0m"
#define TERMINAL_GREEN "\033[92m"
#define TERMINAL_BLUE "\033[94m"
#define TERMINAL_HEADER "\033[95m"
#define TERMINAL_YELLOW "\033[93m"
#define TERMINAL_RED "\033[91m"

#define CONCLUDE_TEST_SUITE             \
uint64_t __time__ = ((time_stamp()-     \
                      __start__)/1000); \
std::cout << std::endl                  \
          << __passed_test_cases__      \
          << " test case(s) passed"     \
          << std::endl;                 \
if (__inconclusive_test_cases__ != 0)   \
{                                       \
    std::cout << __inconclusive_test_cases__ \
              << " test case(s) "       \
              << "inconclusive "        \
              << std::endl;             \
}                                       \
std::cout << __failed_test_cases__      \
          << " test case(s) failed"     \
          << std::endl;                 \
size_t __not_run__ =                    \
    __amount_of_test_cases__ -          \
    __failed_test_cases__ -             \
    __passed_test_cases__ -             \
    __inconclusive_test_cases__;        \
std::cout << __not_run__                \
          << " test case(s) excluded"   \
          << std::endl                  \
          << "Total run time: "         \
          << __time__                   \
          << " ms"                      \
          << std::endl;                 \
if (__warning_test_cases__ != 0)        \
{                                       \
    std::cout << std::endl              \
              << __warning_test_cases__ \
              << " test case(s) "       \
              << "had warning(s) "      \
              << std::endl;             \
}                                       \
std::cout << std::endl;                 \
if (__amount_of_test_cases__ ==         \
    __passed_test_cases__)              \
{                                       \
    exit(0);                            \
}                                       \
exit(1);

#define ADD_TEST_CASE(group, tc)        \
do {                                    \
    __tc__[#group].push_back(tc);       \
    ++__amount_of_test_cases__;         \
}                                       \
while (0)

#define RUN_GROUP(group) do {run_group(__tc__, group); } while (0)

#define RUN_ALL run_all(__tc__)

#define RUN run(__tc__, argc, argv)

void run_group(tc_container& container, const char* group)
{
    if (container.count(group) == 0)
    {
        std::cout << TERMINAL_RED
                  << "ERROR: Group '" << group
                  << "' contains zero test cases"
                  << TERMINAL_END
                  << std::endl;
        return;
    }
    std::cout << "Group: " << group << std::endl;
    for (auto i : container[group])
    {
        i();
    }
    std::cout << std::endl;
}

void run_groups(tc_container& container, const char* groups)
{
    char txt[1000];
    strcpy(txt, groups);
    char* group = txt;
    while (true)
    {
        char* next_group = strchr(group, ',');
        if (next_group == NULL)
        {
            run_group(container, group);
            break;
        }
        *next_group = '\0';
        run_group(container, group);
        group = next_group + 1;
    }
}

void run_all(tc_container& container)
{
    for (auto i : container)
    {
        run_group(container, i.first.data());
    }
}

void run(tc_container& container, int argc, char** argv)
{
    bool run_all_flag = true;
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0)
        {
            char* program = strrchr(argv[0], '/') + 1;
            std::cout << "Usage: " << program << std::endl
                      << "-a --all\t Run all test cases" << std::endl
                      << "-g --group=<G>\t Run all test cases from <G>" << std::endl
                      << "-e --max-erros\t Terminate a test case after this amount of errors (default 10)" << std::endl
                      << std::endl
                      << "Groups:" << std::endl;
            size_t max_size = 0;
            for (auto i : container)
            {
                max_size = std::max(max_size, i.first.length());
            }
            for (auto i : container)
            {
                std::cout << i.first;
                for (size_t n = i.first.length(); n < max_size + 5; ++n)
                {
                    std::cout << " ";
                }
                std::cout << i.second.size() << std::endl;
            }
            std::cout << std::endl;
            exit(0);
        }
        else if (strcmp(argv[i], "-a") == 0 ||
            strcmp(argv[i], "--all") == 0)
        {
            run_all(container);
            run_all_flag = false;
        }
        else if (strcmp(argv[i], "-g") == 0 ||
                 strcmp(argv[i], "--group") == 0)
        {
            run_groups(container, argv[++i]);
            run_all_flag = false;
        }
        else if (strncmp(argv[i], "-g=", 3) == 0 ||
                 strncmp(argv[i], "--group=", 8) == 0)
        {
            char* group = strchr(argv[i], '=') + 1;
            run_groups(container, group);
            run_all_flag = false;
        }
        else if (strcmp(argv[i], "-e") == 0 ||
                 strcmp(argv[i], "--max-errors") == 0)
        {
            __max_errors__ = atoi(argv[++i]);
        }
        else if (strncmp(argv[i], "-e=", 3) == 0 ||
                 strncmp(argv[i], "--max-errors=", 13) == 0)
        {
            __max_errors__ = atoi(strchr(argv[i], '=') + 1);
        }
        else
        {
            std::cout << "Error: Unknown parameter '"
                      << argv[i] << "'" << std::endl;
        }
    }
    if (run_all_flag)
    {
        run_all(container);
    }
}
