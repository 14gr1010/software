#include "suite.hpp"

INIT_TEST_SUITE;

void signal_int(int /* sig */)
{
    std::cout << std::endl;
    CONCLUDE_TEST_SUITE;
    exit(1);
}

int main(int argc, char** argv)
{
    signal(SIGINT, signal_int);

    ADD_TEST_CASE(misc, failing_test_case);
    ADD_TEST_CASE(misc, passing_test_case);

    RUN;

    CONCLUDE_TEST_SUITE;
}
