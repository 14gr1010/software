#include "delay_pkt.hpp"

#include <cstring>

#include "../test_case.hpp"

#include "layers/delay.hpp"
#include "layers/delay_toggle.hpp"
#include "layers/in_order.hpp"
#include "layers/final_layer.hpp"

void delay_pkt_1()
{
    INIT_TEST_CASE;

    typedef delay<final_layer> delayer;

    size_t length = 31;
    double latency_start = 0.001;
    double latency_stop = 0.1;
    double latency_inc = 0.02;
    int n = -1;

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();

    B->data_push(length);
    memset(B->head, 0x30, B->len);

    delayer* d = new delayer();

    for (double latency = latency_start;
         latency < latency_stop; latency += latency_inc)
    {
        d->parse({ {"delay::tx", latency}});

        A->clear();
        A->id = ++n;
        A->data_push(length);
        memset(A->head, 0x30, A->len);

        uint64_t start = time_stamp();

        while (d->write_pkt(A) == 0);

        uint64_t end = time_stamp();

        double tolerance = 0.02;
        TEST_GTE(latency, (1-tolerance) * (end-start)/1000000.0);
        TEST_LTE(latency, (1+tolerance) * (end-start)/1000000.0);

        int r = memcmp(A->head, B->head, length);
        if (r != 0)
        {
            A->print_head(A->len, "A");
            B->print_head(B->len, "B");
        }
        TEST_EQ(r, 0);

        TEST_EQ(A->len, B->len);
    }
    
    delete d;

    CONCLUDE_TEST_CASE;
}

void delay_pkt_2()
{
    INIT_TEST_CASE;

    typedef delay<final_layer> delayer;

    size_t length = 31;
    int latency_start = 1;
    int latency_stop = 100;
    int latency_inc = 1;
    int n = -1;

    delayer* d = new delayer();

    std::map<T_pkt_id, uint64_t> start;
    std::map<T_pkt_id, uint64_t> end;
    std::map<T_pkt_id, uint64_t> diff;
    std::map<T_pkt_id, double> target_latency;
    int i = latency_start;
    p_pkt_buffer A = new_pkt_buffer();
    bool run = true;
    while (run)
    {
        if (i <= latency_stop)
        {
            double latency = i/1000.;
            d->parse({ {"delay::tx", latency}});
            T_pkt_id id = ++n;
            A->clear();
            A->id = id;
            A->data_push(length);

            target_latency[id] = latency;

            assert(start.count(id) == 0);
            start[id] = time_stamp();

            d->write_pkt(A);
            i += latency_inc;
        }

        A->clear();
        d->write_pkt(A);
        if (!A->is_empty())
        {
            T_pkt_id id = A->id;

            assert(end.count(id) == 0);
            end[id] = time_stamp();
            diff[id] = end[id] - start[id];
            if (end.size() == start.size())
            {
                run = false;
            }
        }
    }

    for (auto i = diff.cbegin(); i != diff.cend(); ++i)
    {
        T_pkt_id id = i->first;

        double tolerance = 0.01;
        TEST_GTE(target_latency[id], (1-tolerance)*(diff[id])/1000000.0);
        TEST_LTE(target_latency[id], (1+tolerance)*(diff[id])/1000000.0);
    }

    delete d;

    CONCLUDE_TEST_CASE;
}

void delay_pkt_3()
{
    INIT_TEST_CASE;

    typedef delay_toggle<final_layer> delayer;

    size_t length = 31;
    size_t N = 43;
    int D = 7;

    delayer* d = new delayer();

    for (int i = 0; i < D; ++i)
    {
        d->set_tx_delay(i, 0.01*(i+1));
    }

    std::map<T_pkt_id, uint64_t> start;
    std::map<T_pkt_id, uint64_t> end;
    std::map<T_pkt_id, uint64_t> diff;
    std::map<T_pkt_id, double> target_latency;
    p_pkt_buffer A = new_pkt_buffer();
    bool run = true;
    size_t i = 0;
    T_pkt_id n = 0;
    while (run)
    {
        if (i < N)
        {
            T_pkt_id id = ++n;
            A->clear();
            A->id = id;
            A->data_push(length);

            double latency = (((i+1)%D)+1)*0.01;
            target_latency[id] = latency;

            assert(start.count(id) == 0);
            start[id] = time_stamp();

            d->write_pkt(A);
            ++i;
        }

        A->clear();
        d->write_pkt(A);
        if (!A->is_empty())
        {
            T_pkt_id id = A->id;

            assert(end.count(id) == 0);
            end[id] = time_stamp();
            diff[id] = end[id] - start[id];
            if (end.size() == start.size())
            {
                run = false;
            }
        }
    }

    for (auto i = diff.cbegin(); i != diff.cend(); ++i)
    {
        T_pkt_id id = i->first;

        double tolerance = 0.01;
        TEST_GTE(target_latency[id], (1-tolerance) * (diff[id])/1000000.0);
        TEST_LTE(target_latency[id], (1+tolerance) * (diff[id])/1000000.0);
    }

    delete d;

    CONCLUDE_TEST_CASE;
}

void delay_pkt_4()
{
    INIT_TEST_CASE;

    typedef in_order<delay_toggle<final_layer>> delayer;

    size_t length = 31;
    size_t N = 43;
    int D = 7;

    delayer* d = new delayer({ {"in_order::timeout", 0.01*(D+1)},
                               {"in_order::use_fixed_timeout", true} });

    for (int i = 0; i < D; ++i)
    {
        d->set_tx_delay(i, 0.01*(i+1));
    }

    std::map<T_pkt_id, uint64_t> start;
    std::map<T_pkt_id, uint64_t> end;
    std::map<T_pkt_id, uint64_t> diff;
    std::map<T_pkt_id, double> target_latency;
    p_pkt_buffer A = new_pkt_buffer();
    bool run = true;
    size_t i = 0;
    T_pkt_id last_id = 0;
    while (run)
    {
        if (i < N)
        {
            A->clear();
            A->data_push(length);
            d->read_pkt(A);
            T_pkt_id id = A->id;

            double latency = (((i+1)%D)+1)*0.01;
            target_latency[id] = latency;

            assert(start.count(id) == 0);
            start[id] = time_stamp();

            d->write_pkt(A);
            ++i;
        }

        A->clear();
        d->write_pkt(A);
        if (!A->is_empty())
        {
            T_pkt_id id = A->id;

            TEST_EQ(++last_id, id);

            assert(end.count(id) == 0);
            end[id] = time_stamp();
            diff[id] = end[id] - start[id];

            TEST_GTE(diff[id], target_latency[id]);

            if (end.size() == start.size())
            {
                run = false;
            }
        }
    }
    
    delete d;

    CONCLUDE_TEST_CASE;
}
