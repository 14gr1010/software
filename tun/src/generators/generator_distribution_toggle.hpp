#pragma once

#include "misc.hpp"

class generator_distribution_toggle
{
    struct T_priority_and_count
    {
        int priority;
        int count;
    };

    int index = 0;
    std::map<int, T_priority_and_count> priorities;

    virtual size_t get_generator_max() = 0;

public:
    int get_index()
    {
        if (priorities[index].count < priorities[index].priority)
        {
            ++priorities[index].count;
            return index;
        }
        priorities[index].count = 0;
        index = (index + 1) % get_generator_max();
        return get_index();
    }

    void set_distribution(int index, int priority)
    {
        ASSERT_EQ(0, priorities.count(index));
        ASSERT_LT(0, priority);

        priorities[index].priority = priority;
    }
};
