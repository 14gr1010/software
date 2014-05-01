#include "kodo.hpp"

#include <cstring>

#include "../test_case.hpp"

#include "layers/write_export.hpp"
#include "layers/kodo_fixed_redundancy.hpp"
#include "layers/kodo_on_the_fly_redundancy.hpp"
#include "layers/kodo_fixed_simple_redundancy.hpp"
#include "layers/kodo_on_the_fly_simple_redundancy.hpp"
#include "layers/kodo_recoder.hpp"
#include "layers/final_layer.hpp"

void kodo_1()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_fixed_redundancy<
            final_layer
            >> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        TEST_EQ(0, c->get_rx_delivered_symbols(c->get_rx_gen()));
        c->read_pkt(A);
    }

    TEST_EQ(gen_size, c->get_rx_delivered_symbols(c->get_rx_gen()));
    TEST_EQ(gen_size, Q.size());

    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        int r = memcmp(A->head, Q[i]->head, symbol_size);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, 0);
    }

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_2()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_on_the_fly_redundancy<
            final_layer
            >> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        TEST_EQ(i, c->get_rx_delivered_symbols(c->get_rx_gen()));
        c->read_pkt(A);
        if (i < gen_size-1)
        {
            TEST_EQ(i+1, c->get_rx_delivered_symbols(c->get_rx_gen()));
        }
    }

    TEST_EQ(gen_size, Q.size());

    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        int r = memcmp(A->head, Q[i]->head, symbol_size);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, 0);
    }

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_3()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_fixed_simple_redundancy<
            final_layer
            >> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        TEST_EQ(0, c->get_rx_delivered_symbols());
        c->read_pkt(A);
    }

    TEST_EQ(gen_size, c->get_rx_delivered_symbols());
    TEST_EQ(gen_size, Q.size());

    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        int r = memcmp(A->head, Q[i]->head, symbol_size);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, 0);
    }

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_4()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_on_the_fly_simple_redundancy<
            final_layer
            >> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        TEST_EQ(i, c->get_rx_delivered_symbols());
        c->read_pkt(A);
        TEST_EQ(i+1, c->get_rx_delivered_symbols());
    }

    TEST_EQ(gen_size, c->get_rx_delivered_symbols());
    TEST_EQ(gen_size, Q.size());

    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        int r = memcmp(A->head, Q[i]->head, symbol_size);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, 0);
    }

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_5()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_on_the_fly_redundancy<
            kodo_recoder<
            final_layer
            >>> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        TEST_EQ(i, c->get_rx_delivered_symbols(c->get_rx_gen()));
        c->read_pkt(A);
        if (i < gen_size-1)
        {
            TEST_EQ(i+1, c->get_rx_delivered_symbols(c->get_rx_gen()));
        }
    }

    TEST_EQ(gen_size, Q.size());

    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        int r = memcmp(A->head, Q[i]->head, symbol_size);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, 0);
    }

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_6()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_on_the_fly_redundancy<
            final_layer
            >> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;
    uint32_t generations = 997;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    uint64_t start_time = time_stamp();

    for (uint32_t u = 0; u < generations; ++u)
    {
        p_pkt_buffer A = new_pkt_buffer();
        for (uint32_t i = 0; i < gen_size; ++i)
        {
            A->clear();
            A->data_push(symbol_size);
            memset(A->head, i, symbol_size);
            if (i > 0 || u == 0)
            {
                // We only test after we have put at least one packet into the decoder, except for the first generation
                TEST_EQ(i, c->get_rx_delivered_symbols(c->get_rx_gen()));
            }
            c->read_pkt(A);
            if (i < gen_size-1)
            {
                TEST_EQ(i+1, c->get_rx_delivered_symbols(c->get_rx_gen()));
            }
        }

        TEST_EQ(gen_size, Q.size());

        for (uint32_t i = 0; i < gen_size; ++i)
        {
            A->clear();
            A->data_push(symbol_size);
            memset(A->head, i, symbol_size);
            int r = memcmp(A->head, Q[i]->head, symbol_size);
            if (r != 0)
            {
                A->print_head(A->len, "A");
                Q[i]->print_head(Q[i]->len, "Q");
            }
            TEST_EQ(r, 0);
        }
        Q.clear();
    }

    uint64_t end_time = time_stamp();
    TEST_LTE(end_time - start_time, 135000UL);

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_7()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_on_the_fly_redundancy<
            kodo_recoder<
            final_layer
            >>> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;
    uint32_t generations = 997;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    uint64_t start_time = time_stamp();

    for (uint32_t u = 0; u < generations; ++u)
    {
        p_pkt_buffer A = new_pkt_buffer();
        for (uint32_t i = 0; i < gen_size; ++i)
        {
            A->clear();
            A->data_push(symbol_size);
            memset(A->head, i, symbol_size);
            if (i > 0 || u == 0)
            {
                // We only test after we have put at least one packet into the decoder, except for the first generation
                TEST_EQ(i, c->get_rx_delivered_symbols(c->get_rx_gen()));
            }
            c->read_pkt(A);
            if (i < gen_size-1)
            {
                TEST_EQ(i+1, c->get_rx_delivered_symbols(c->get_rx_gen()));
            }
        }

        TEST_EQ(gen_size, Q.size());

        for (uint32_t i = 0; i < gen_size; ++i)
        {
            A->clear();
            A->data_push(symbol_size);
            memset(A->head, i, symbol_size);
            int r = memcmp(A->head, Q[i]->head, symbol_size);
            if (r != 0)
            {
                A->print_head(A->len, "A");
                Q[i]->print_head(Q[i]->len, "Q");
            }
            TEST_EQ(r, 0);
        }
        Q.clear();
    }

    uint64_t end_time = time_stamp();
    TEST_LTE(end_time - start_time, 315000UL);

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_8()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_fixed_redundancy<
            final_layer
            >> coder;

    uint32_t gen_size = 31;
    uint32_t symbol_size = 101;
    uint32_t generations = 997;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size},
                           {"kodo::tx_symbol_size", symbol_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    uint64_t start_time = time_stamp();

    for (uint32_t u = 0; u < generations; ++u)
    {
        p_pkt_buffer A = new_pkt_buffer();
        for (uint32_t i = 0; i < gen_size; ++i)
        {
            A->clear();
            A->data_push(symbol_size);
            memset(A->head, i, symbol_size);
            if (u == 0)
            {
                TEST_EQ(0, c->get_rx_delivered_symbols(c->get_rx_gen()));
            }
            else
            {
                TEST_EQ(gen_size, c->get_rx_delivered_symbols(c->get_rx_gen()));
            }
            c->read_pkt(A);
        }

        TEST_EQ(gen_size, c->get_rx_delivered_symbols(c->get_rx_gen()));
        TEST_EQ(gen_size, Q.size());

        for (uint32_t i = 0; i < gen_size; ++i)
        {
            A->clear();
            A->data_push(symbol_size);
            memset(A->head, i, symbol_size);
            int r = memcmp(A->head, Q[i]->head, symbol_size);
            if (r != 0)
            {
                A->print_head(A->len, "A");
                Q[i]->print_head(Q[i]->len, "Q");
            }
            TEST_EQ(r, 0);
        }
        Q.clear();
    }

    uint64_t end_time = time_stamp();
    TEST_LTE(end_time - start_time, 110000UL);

    delete c;

    CONCLUDE_TEST_CASE;
}

void kodo_9()
{
    INIT_TEST_CASE;

    typedef write_export<
            kodo_on_the_fly_redundancy<
            kodo_recoder<
            final_layer
            >>> coder;

    uint32_t gen_size = 5;
    uint32_t symbol_size = 84;

    coder* c = new coder({ {"kodo::tx_gen_size", gen_size} });
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));

    T_pkt_queue Q;
    c->set_export_buffer(&Q);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        TEST_EQ(i, c->get_rx_delivered_symbols(c->get_rx_gen()));
        c->read_pkt(A);
        if (i < gen_size-1)
        {
            TEST_EQ(i+1, c->get_rx_delivered_symbols(c->get_rx_gen()));
        }
    }

    TEST_EQ(gen_size, Q.size());

    for (uint32_t i = 0; i < gen_size; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        int r = memcmp(A->head, Q[i]->head, symbol_size);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, 0);
    }

    delete c;

    CONCLUDE_TEST_CASE;
}
