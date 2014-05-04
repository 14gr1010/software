#pragma once

// #define VERBOSE 1

#include <map> // std::map
#include <vector> // std::vector
#include <kodo/rlnc/on_the_fly_codes.hpp>

#include "kodo_redundancy.hpp"

template<class super>
class kodo_recoder : public super
{
    typedef kodo::debug_on_the_fly_decoder<fifi::binary8> rlnc_decoder;

    struct decoder_info
    {
        uint64_t last_pkt_time = 0;
        uint64_t first_pkt_time = 0;
        rlnc_decoder::pointer decoder;
        uint32_t symbol_size = 0;
        uint32_t gen_size = 0;
        uint32_t delivered_pkts = 0;
        uint32_t encoder_rank = 0;
    };

    // Transmitter
    double tx_redundancy = 1.167;

    // Receiver
    uint32_t rx_max_gen_size = 100;
    uint32_t rx_max_symbol_size = 1360;
    rlnc_decoder::factory decoder_factory;
    std::map<uint32_t, decoder_info> decoders;
    double rx_pkt_timeout = 0.001;
    const int max_inter_pkt_delays = 15;
    std::vector<uint64_t> inter_pkt_delays;
    int inter_pkt_delays_index = 0;
    size_t max_amount_of_undelivered_generations = 1000;

public:
    kodo_recoder(T_arg_list args = {})
    : super(args),
      decoder_factory(rx_max_gen_size, rx_max_symbol_size)
    {
        (void)set_packet_zeros; // UNUSED!
        (void)get_packet_zeros; // UNUSED!

        parse(args, false);
        inter_pkt_delays.resize(max_inter_pkt_delays);
    }

    ~kodo_recoder()
    {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Recoder: size=%lu rank=%u, symbol size=%u\n", decoders.size(), (decoders.size() == 0 ? 0 : decoders.begin()->second.decoder->rank()), (decoders.size() == 0 ? 0 : decoders.begin()->second.symbol_size));
#endif
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        rx_pkt_timeout = get_arg("kodo::rx_pkt_timeout", rx_pkt_timeout, args).d;
        tx_redundancy = get_arg("kodo::tx_redundancy", tx_redundancy, args).d;
        ASSERT_GTE(tx_redundancy, 1);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        update_rx_pkt_timeout();

        uint32_t pkt_encoder_rank = get_packet_field<uint32_t>(buf);
        uint32_t pkt_gen = get_packet_gen(buf);
        uint32_t pkt_gen_size = get_packet_gen_size(buf);
        uint32_t pkt_symbol_size = get_packet_symbol_size(buf);

        if (decoders.count(pkt_gen) == 0)
        {
            rx_new_generation(pkt_gen, pkt_gen_size, pkt_symbol_size);
        }

        decoder_info* d = &decoders[pkt_gen];

        ASSERT_EQ(pkt_gen_size, d->gen_size);
        ASSERT_EQ(pkt_symbol_size, d->symbol_size);


        d->encoder_rank = std::max(d->encoder_rank, pkt_encoder_rank);
        ASSERT_NEQ(d->encoder_rank, 0);
        ASSERT_LTE(d->encoder_rank, d->gen_size);

        p_pkt_buffer systematic_pkt;
        bool forward_systematic_packet = (buf->len == (9 + pkt_symbol_size));
        if (true == forward_systematic_packet)
        {
            systematic_pkt = copy_pkt_buffer(buf);
        }

        if (!d->decoder->is_complete())
        {
            d->last_pkt_time = time_stamp();
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            uint32_t rank_before = d->decoder->rank();
#endif
            d->decoder->decode(buf->head);
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Adding pkt of %lu bytes to recoder, rank before: %u, rank after: %u\n", buf->len, rank_before, d->decoder->rank());
            d->decoder->print_cached_symbol_coefficients(std::cout);
#endif
        }

        if (true == forward_systematic_packet)
        {
            ++d->delivered_pkts;

#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Sending TX generation, gen=%u, pkts=%u\n", pkt_gen, 1);
            printf("Size of KODO (forwarded) symbol %lu bytes, gen=%u\n", systematic_pkt->len, pkt_gen);
#endif
            set_packet_symbol_size(systematic_pkt, d->symbol_size);
            set_packet_gen_size(systematic_pkt, d->gen_size);
            set_packet_gen(systematic_pkt, pkt_gen);
            set_packet_field<uint32_t>(systematic_pkt, d->encoder_rank);

            super::read_pkt(systematic_pkt);
        }

        buf->clear();

#if defined(VERBOSE) || defined(KODO_VERBOSE)
        uint32_t tmp = (d->decoder->rank() * tx_redundancy) - d->delivered_pkts;
        printf("Amount of recoded pkts: %u, rank=%u, tx_redundancy=%f, delivered_pkts=%u, gensize=%u \n", tmp, d->decoder->rank(), tx_redundancy, d->delivered_pkts, d->gen_size);
#endif
        if (d->decoder->rank() * tx_redundancy > d->delivered_pkts)
        {
            uint32_t amount_of_pkts = (d->decoder->rank() * tx_redundancy) - d->delivered_pkts;

            if (d->decoder->rank() == d->gen_size)
            {
                amount_of_pkts = std::ceil(d->decoder->rank() * tx_redundancy) - d->delivered_pkts;
            }
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Sending TX generation, gen=%u, pkts=%u\n", pkt_gen, amount_of_pkts);
#endif
            d->delivered_pkts += amount_of_pkts;
            for (uint32_t i = 0; i < amount_of_pkts; ++i)
            {
                size_t len = d->decoder->recode(buf->head);
                sak::big_endian::put<uint32_t>(d->decoder->seen_encoder_rank(), buf->head);
                buf->data_push(len);
                buf->data_len = d->symbol_size;

#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Size of KODO (recoded) symbol %lu bytes, gen=%u\n", len, pkt_gen);
#endif
                set_packet_symbol_size(buf, d->symbol_size);
                set_packet_gen_size(buf, d->gen_size);
                set_packet_gen(buf, pkt_gen);
                set_packet_field<uint32_t>(buf, d->encoder_rank);

                super::read_pkt(buf);

                buf->clear();
            }
            return d->symbol_size;
        }

        return 0;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            garbage_collection();
        }

        return super::write_pkt(buf);
    }

    uint32_t get_rx_gen_size(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        return decoders[gen].gen_size;
    }

    uint32_t get_rx_symbol_size(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        return decoders[gen].symbol_size - sizeof(T_zeropad);
    }

    uint32_t get_rx_gen() const
    {
        if (decoders.size() == 0)
        {
            return 0;
        }
        return decoders.crbegin()->first;
    }

private:
    void update_rx_pkt_timeout()
    {
        static uint64_t now = 0;
        static uint64_t last = 0;
        last = now;
        now = time_stamp();
        inter_pkt_delays[inter_pkt_delays_index] = now - last;
        inter_pkt_delays_index =
            (inter_pkt_delays_index + 1) % max_inter_pkt_delays;

        uint64_t total_inter_pkt_delay = 0;
        for (uint64_t i : inter_pkt_delays)
        {
            total_inter_pkt_delay += std::min(std::max(i, (uint64_t)20),
                                              (uint64_t)500);
        }
        rx_pkt_timeout =
            total_inter_pkt_delay/((double)max_inter_pkt_delays*1000000);
        rx_pkt_timeout *= 1.05;
    }

    void rx_new_generation(uint32_t gen, uint32_t gen_size, uint32_t symbol_size)
    {
        ASSERT_LTE(gen_size, rx_max_gen_size);
        ASSERT_LTE(symbol_size, rx_max_symbol_size);
        ASSERT_NEQ(gen_size, 0);
        ASSERT_NEQ(symbol_size, 0);

        decoder_info d;
        d.first_pkt_time = time_stamp();
        decoder_factory.set_symbols(gen_size);
        decoder_factory.set_symbol_size(symbol_size);
        d.decoder = decoder_factory.build();
        d.gen_size = gen_size;
        d.symbol_size = symbol_size;
        d.delivered_pkts = 0;
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        uint32_t last_rank = 0;
        uint32_t delivered_symbols = 0;
        if (decoders.size() != 0)
        {
            last_rank = decoders.crbegin()->second.decoder->rank();
            delivered_symbols =
                decoders.crbegin()->second.delivered_pkts;
        }
        printf("RX gen=%u gs=%u ss=%u last gen: rank=%u del=%u\n",
               gen, gen_size, symbol_size, last_rank, delivered_symbols);
#endif
        decoders[gen] = d;
        while (decoders.size() > max_amount_of_undelivered_generations)
        {
            decoders.erase(decoders.begin());
        }
    }

    uint32_t garbage_collection()
    {
        bool multiple_decoders = decoders.size() > 1;
        uint64_t now = time_stamp();
        uint64_t rx_pkt_timeout_us = rx_pkt_timeout * 1000000 * 100; // DEBUG!!
        for (auto i = decoders.begin(); i != decoders.end(); /* NO INCREMENT */)
        {
            uint64_t time_out_time_first = rx_pkt_timeout_us + i->second.last_pkt_time;
            uint64_t time_out_time_last = rx_pkt_timeout_us * i->second.gen_size + i->second.first_pkt_time;
            bool timed_out = time_out_time_last < now && time_out_time_first < now;
            if (timed_out && multiple_decoders)
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Erasing gen %u due to time out, TTL %lu, del=%u/%u/%u, gen was %scomplete\n",
                       i->first, now-i->second.first_pkt_time,
                       i->second.delivered_pkts,
                       i->second.decoder->symbols_decoded(),
                       i->second.decoder->rank(),
                       (i->second.decoder->is_complete() ? "" : "NOT "));
#endif
                decoders.erase(i++);
            }
            else
            {
                ++i;
            }
        }
        return 0;
    }
};
