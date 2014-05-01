#include "stack_args.hpp"

#include "../test_case.hpp"

#include "layers/xor_adaptive_redundancy.hpp"
#include "layers/final_layer.hpp"

void args_1()
{
    INIT_TEST_CASE;

    typedef xor_fixed_redundancy<final_layer> xor_redundancy;

    uint8_t amount = 13;
    xor_redundancy* x = new xor_redundancy({ {"xor_fixed_redundancy::amount", amount} });
    TEST_EQ(x->get_redundancy(), amount);
    x->parse({ {"xor_fixed_redundancy::amount", amount*2} });
    TEST_NEQ(x->get_redundancy(), amount);
    TEST_EQ(x->get_redundancy(), amount*2);

    delete x;

    CONCLUDE_TEST_CASE;
}

void args_2()
{
    INIT_TEST_CASE;

    typedef xor_adaptive_redundancy<final_layer> xora_redundancy;

    uint8_t amount = 7;
    xora_redundancy* x =  new xora_redundancy({ {"xor_fixed_redundancy::amount", amount} });
    TEST_EQ(x->get_redundancy(), amount);
    x->parse({ {"xor_fixed_redundancy::amount", amount*2} });
    TEST_NEQ(x->get_redundancy(), amount);
    TEST_EQ(x->get_redundancy(), amount*2);

    delete x;

    CONCLUDE_TEST_CASE;
}
