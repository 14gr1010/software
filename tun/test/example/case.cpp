#include "case.hpp"

void failing_test_case()
{
    INIT_TEST_CASE;

    int one = 1, two = 2;

    TEST_EQ(one, two);
    TEST_GT(one, two);
    TEST_GTE(one, two);
    TEST_LT(two, one);
    TEST_LTE(two, one);

    CONCLUDE_TEST_CASE;
}

void passing_test_case()
{
    INIT_TEST_CASE;

    int one = 1, two = 2;

    TEST_EQ(one, one);
    TEST_GT(two, one);
    TEST_GTE(two, one);
    TEST_GTE(two, two);
    TEST_LT(one, two);
    TEST_LTE(one, one);
    TEST_LTE(one, two);

    CONCLUDE_TEST_CASE;
}
