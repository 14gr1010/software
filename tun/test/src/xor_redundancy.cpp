#include "xor_redundancy.hpp"

#include <cstring>
#include <algorithm>

#include "../test_case.hpp"

#include "layers/write_export.hpp"
#include "layers/in_order.hpp"
#include "layers/xor_fixed_redundancy.hpp"
#include "layers/final_layer.hpp"

void xor_redundancy_1()
{
    INIT_TEST_CASE;

    typedef write_export<
            xor_fixed_redundancy<
            final_layer
            >> coder;
    
    uint32_t symbol_size = 101;
    uint32_t N = 100003;
    
    coder* c = new coder({ {"xor_fixed_redundancy::amount", 2} });
    
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));
    
    T_pkt_queue Q;
    c->set_export_buffer(&Q);
    
    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < N; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        c->read_pkt(A);
    }

    TEST_EQ(N, Q.size());

    uint64_t start_time = time_stamp();

    for (uint32_t i = 0; i < N; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        bool r = std::equal(A->head, A->head+symbol_size, Q[i]->head);
        if (r != true)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, true);
    }
    
    uint64_t end_time = time_stamp();
    TEST_LTE(end_time - start_time, 12000UL);
    
    delete c;

    CONCLUDE_TEST_CASE;
}

void xor_redundancy_2()
{
    INIT_TEST_CASE;

    typedef write_export<
            in_order<
            xor_fixed_redundancy<
            final_layer
            >>> coder;
    
    uint32_t symbol_size = 101;
    uint32_t N = 100003;
    
    coder* c = new coder({ {"xor_fixed_redundancy::amount", 2} });
    
    c->connect_signal_fw(boost::bind(&coder::write_pkt, c, _1));
    
    T_pkt_queue Q;
    c->set_export_buffer(&Q);
    
    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < N; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        c->read_pkt(A);
    }

    TEST_EQ(N, Q.size());

    uint64_t start_time = time_stamp();

    for (uint32_t i = 0; i < N; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        bool r = std::equal(A->head, A->head+symbol_size, Q[i]->head);
        if (r != true)
        {
            A->print_head(A->len, "A");
            Q[i]->print_head(Q[i]->len, "Q");
        }
        TEST_EQ(r, true);
    }
    
    uint64_t end_time = time_stamp();
    TEST_LTE(end_time - start_time, 12000UL);
    
    delete c;

    CONCLUDE_TEST_CASE;
}

void xor_redundancy_3()
{
    INIT_TEST_CASE;

    typedef write_export<
            in_order<
            xor_fixed_redundancy<
            final_layer
            >>> coder;

    typedef write_export<
            final_layer
            > exporter;

    
    uint32_t symbol_size = 19;
    uint32_t redundancy = 3;
    uint32_t N = 997;
    
    coder* c = new coder({ {"xor_fixed_redundancy::amount", redundancy} });
    exporter* x = new exporter({});
    
    c->connect_signal_fw(boost::bind(&exporter::write_pkt, x, _1));
    
    T_pkt_queue cQ;
    c->set_export_buffer(&cQ);

    T_pkt_queue xQ;
    x->set_export_buffer(&xQ);

    p_pkt_buffer A = new_pkt_buffer();
    for (uint32_t i = 0; i < N; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        c->read_pkt(A);
    }

    TEST_EQ(N/redundancy+N, xQ.size());

    while (xQ.size() > 0)
    {
        c->write_pkt(xQ.back());
        xQ.pop_back();
    }

    for (uint32_t i = 0; i < N-1; ++i)
    {
        A->clear();
        c->write_pkt(A);
    }

    TEST_EQ(N, cQ.size());

    for (uint32_t i = 0; i < N; ++i)
    {
        A->clear();
        A->data_push(symbol_size);
        memset(A->head, i, symbol_size);
        bool r = std::equal(A->head, A->head+symbol_size, cQ[i]->head);
        if (r != true)
        {
            A->print_head(A->len, "A");
            cQ[i]->print_head(cQ[i]->len, "Q");
        }
        TEST_EQ(r, true);
    }
    
    delete c;
    delete x;

    CONCLUDE_TEST_CASE;
}
