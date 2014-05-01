#pragma once

#include "misc.hpp"

class generator_toggle
{
    int index = -1;

    virtual size_t get_generator_max() = 0;

public:
    int get_index()
    {
        index = (index + 1) % get_generator_max();
        return index;
    }
};
