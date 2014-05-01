#pragma once

#include <initializer_list> // std::initializer_list
#include <string> // std::string

union T_argv
{
    int i;
    double d;
    bool b;
    const char* c;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    size_t sizet;

    T_argv() = delete;
    T_argv(int _i) : i(_i) {}
    T_argv(double _d) : d(_d) {}
    T_argv(bool _b) : b(_b) {}
    T_argv(const char* _c) : c(_c) {}
    T_argv(uint8_t _u8) : u8(_u8) {}
    T_argv(uint16_t _u16) : u16(_u16) {}
    T_argv(uint32_t _u32) : u32(_u32) {}
    T_argv(size_t _sizet) : sizet(_sizet) {}
};

typedef std::string T_key;

struct T_arg
{
    T_key key;
    T_argv val;
    
    T_arg(T_key _key, T_argv _val) : key(_key), val(_val) {}
};

typedef std::initializer_list<T_arg> T_arg_list;

inline T_argv get_arg(T_key key, T_argv val, T_arg_list args)
{
    for (T_arg arg : args)
    {
        if (arg.key == key)
        {
            return arg.val;
        }
    }
    return val;
}

