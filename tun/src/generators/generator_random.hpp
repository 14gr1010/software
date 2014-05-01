#pragma once

#include <cstdlib> // rand()

#include "misc.hpp"

class generator_random
{
    int index = -1;

    virtual size_t get_generator_max() = 0;

public:
    int get_index()
    {
        index = rand() % get_generator_max();
        return index;
    }
};
