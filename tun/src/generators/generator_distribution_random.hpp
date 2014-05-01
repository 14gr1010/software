#pragma once

#include <cstdlib> // rand()
#include <map> // std::map

#include "misc.hpp"

class generator_distribution_random
{
    int index = 0;
    int probability_sum = 0;
    std::map<int, int> probabilities;

    virtual size_t get_generator_max() = 0;

public:
    int get_index()
    {
        int p = (rand() % 100) + 1;
        int sum = 0;
        for (auto i : probabilities)
        {
            sum += i.second;
            if (p <= sum)
            {
                return i.first;
            }
        }

        ASSERT_EQ(probability_sum, 100);

        return -1;
    }

    void set_distribution(int index, int probability)
    {
        ASSERT_EQ(0, probabilities.count(index));
        ASSERT_LT(0, probability);

        probabilities[index] = probability;
        probability_sum += probability;

        ASSERT_LTE(probability_sum, 100);
    }
};
