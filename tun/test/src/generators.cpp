#include "generators.hpp"

#include <vector>

#include "../test_case.hpp"

#include "generators/generator_toggle.hpp"
#include "generators/generator_distribution_toggle.hpp"
#include "generators/generator_random.hpp"
#include "generators/generator_distribution_random.hpp"

template <class generator>
class toggle_max : public generator
{
    int max;

    size_t get_generator_max()
    {
        return max;
    }

public:
    toggle_max(int num)
    {
        max = num;
    }
};

void generator_toggle_1()
{
    INIT_TEST_CASE;

    int levels = 29;
    int N = 3;

    for (int m = 1; m < levels; ++m)
    {
        toggle_max<generator_toggle> t(m);
        for (int i = 0; i < m*N; ++i)
        {
            TEST_EQ(t.get_index(), (i%m));
        }
    }

    CONCLUDE_TEST_CASE;
}

void generator_toggle_2()
{
    INIT_TEST_CASE;

    int levels = 29;
    std::vector<int> checklist;

    for (int m = 1; m < levels; ++m)
    {
        toggle_max<generator_distribution_toggle> t(m);

        checklist.clear();

        for (int n = 0; n < m; ++n)
        {
            t.set_distribution(n, n + 1);

            for (int i = 0; i < n + 1; ++i)
            {
                checklist.push_back(n);
            }
        }

        int index = t.get_index();
        int count = 0;
        TEST_EQ(checklist[count], index);
        ++count;

        while ((index = t.get_index()) > 0)
        {
            TEST_EQ(checklist[count], index);
            ++count;
        }

        TEST_EQ(count, (m*(m+1))/2);
    }

    CONCLUDE_TEST_CASE;
}

void generator_random_1()
{
    INIT_TEST_CASE;

    int levels = 11;
    int N = 200000;
    int index = -1;
    std::vector<int> count;
    count.reserve(levels);

    for (int m = 1; m < levels; ++m)
    {
        toggle_max<generator_random> t(m);
        count.clear();
        count.resize(m);
        for (int i = 0; i < N; ++i)
        {
            index = t.get_index();
            ++count[index];
        }

        double tolerance = 0.03;
        for (int i : count)
        {
            TEST_GTE(i, (1-tolerance) * N/m);
            TEST_LTE(i, (1+tolerance) * N/m );
        }
    }

    CONCLUDE_TEST_CASE;
}

void generator_random_2()
{
    INIT_TEST_CASE;

    int levels = 11;
    int N = 100000;
    int index = -1;
    std::vector<int> count;
    count.reserve(levels);

    for (int m = 1; m < levels; ++m)
    {
        toggle_max<generator_distribution_random> t(m);
        
        for (int i = 0; i < m-1; ++i)
        {
            t.set_distribution(i, 100/m);
        }
        t.set_distribution(m-1, 100/m+100%m);

        count.clear();
        count.resize(m);
        for (int i = 0; i < N; ++i)
        {
            index = t.get_index();
            ++count[index];
        }

        double tolerance = 0.03;
        for (int i = 0; i < m-1; ++i)
        {
            TEST_GTE(count[i], (1-tolerance) * (N*(100/m))/100);
            TEST_LTE(count[i], (1+tolerance) * (N*(100/m))/100);
        }
        TEST_GTE(count[m-1], (1-tolerance) * (N*(100/m+100%m))/100);
        TEST_LTE(count[m-1], (1+tolerance) * (N*(100/m+100%m))/100);
    }

    CONCLUDE_TEST_CASE;
}
