#include "drop_pkt.hpp"

#include "../test_case.hpp"

#include "layers/drop.hpp"
#include "layers/final_layer.hpp"

void drop_rate()
{
    INIT_TEST_CASE;

    typedef drop<final_layer> dropper;

    size_t N = 100000;
    double rate;

    for (rate = 0; rate < 1.01; rate += 0.099)
    {
        dropper* d = new dropper({ {"drop::rate", rate} });

        size_t r = 0;
        size_t w = 0;

        p_pkt_buffer buf = new_pkt_buffer();
        for (size_t n = 0; n < N; ++n)
        {
            buf->clear();
            buf->data_push(1);
            r += 1 - d->read_pkt(buf);
            buf->clear();
            buf->data_push(1);
            w += 1 - d->write_pkt(buf);
        }
        
        double tolerance = 0.01;
        TEST_GTE(r, (1-tolerance) * rate * N);
        TEST_LTE(r, (1+tolerance) * rate * N);
        TEST_GTE(w, (1-tolerance) * rate * N);
        TEST_LTE(w, (1+tolerance) * rate * N);
        
        delete d;
    }

    CONCLUDE_TEST_CASE;
}
