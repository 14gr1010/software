#pragma once

#include <map> // std::map
#include <vector> // std::vector
#include <fstream> // std::fstream
#include <kodo/rlnc/on_the_fly_codes.hpp>

#include "kodo_redundancy.hpp"

template<class super>
class kodo_on_the_fly_redundancy : public super
{
    typedef kodo::on_the_fly_encoder<fifi::binary8> rlnc_encoder;
    typedef kodo::debug_on_the_fly_decoder<fifi::binary8> rlnc_decoder;

    struct decoder_info
    {
        uint64_t last_pkt_time = 0;
        uint64_t first_pkt_time = 0;
        rlnc_decoder::pointer decoder;
        uint32_t symbol_size = 0;
        uint32_t gen_size = 0;
        uint32_t encoder_rank = 0;
        std::map<uint32_t, bool> delivered_symbols;
    };

    // Transmitter
    uint32_t tx_gen = 0;
    uint32_t tx_symbol_size = 1354 + sizeof(T_zeropad);
    uint32_t tx_max_symbol_size = 1354 + sizeof(T_zeropad);
    uint32_t tx_gen_size = 10;
    uint32_t tx_max_gen_size = 100;
    rlnc_encoder::factory encoder_factory;
    rlnc_encoder::pointer encoder;
    double tx_redundancy = 1.167;
    uint32_t tx_delivered_pkts = 0;

    // Receiver
    uint32_t rx_max_gen_size = 100;
    uint32_t rx_max_symbol_size = 1360;
    rlnc_decoder::factory decoder_factory;
    std::map<uint32_t, decoder_info> decoders;
    double rx_pkt_timeout = 0.001;
    std::map<int, bool> delivered_generations;
    size_t max_amount_of_delivered_generations = 1000;
    size_t max_amount_of_undelivered_generations = 1000;
    uint32_t rx_delivered_pkts = 0;
    uint32_t rx_seen_pkts = 0;

    // Inter packet delay info - Used for garbage collection
    const int max_inter_pkt_delays = 15;
    std::vector<uint64_t> inter_pkt_delays;
    int inter_pkt_delays_index = 0;

    // Debug
    uint32_t last_gen = 0;

public:
    kodo_on_the_fly_redundancy(T_arg_list args = {})
    : super(args),
      encoder_factory(tx_max_gen_size, tx_max_symbol_size),
      decoder_factory(rx_max_gen_size, rx_max_symbol_size)
    {
        parse(args, false);
        tx_new_generation();
        inter_pkt_delays.resize(max_inter_pkt_delays);
    }

    ~kodo_on_the_fly_redundancy()
    {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Decoder: seen pkts=%u, delivered pkts=%u, ratio=%f\n", rx_seen_pkts, rx_delivered_pkts, (rx_seen_pkts == 0 ? 1.0 : rx_delivered_pkts/(double)rx_seen_pkts));
        printf("Encoder: rank=%u, symbol size=%u\n", encoder->rank(), tx_symbol_size);
        printf("Decoder: size=%lu rank=%u, symbol size=%u\n", decoders.size(), (decoders.size() == 0 ? 0 : decoders.begin()->second.decoder->rank()), (decoders.size() == 0 ? 0 : decoders.begin()->second.symbol_size));
#endif
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        rx_pkt_timeout = get_arg("kodo::rx_pkt_timeout", {rx_pkt_timeout}, args).d;
        tx_redundancy = get_arg("kodo::tx_redundancy", {tx_redundancy}, args).d;
        ASSERT_GTE(tx_redundancy, 1);
        tx_gen_size = get_arg("kodo::tx_gen_size", {tx_gen_size}, args).u32;
        ASSERT_LTE(tx_gen_size, tx_max_gen_size);
        uint32_t symbol_size = tx_symbol_size - sizeof(T_zeropad);
        tx_symbol_size = get_arg("kodo::tx_symbol_size", {symbol_size}, args).u32 + sizeof(T_zeropad);
        ASSERT_LTE(tx_symbol_size, tx_max_symbol_size);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        ASSERT_LTE(buf->len, tx_symbol_size - sizeof(T_zeropad));

        T_zeropad zeros = buf->zero_pad_to(tx_symbol_size-sizeof(T_zeropad));
        set_packet_zeros(buf, zeros);

#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Adding %hu bytes of zeropadding\n", zeros);
#endif

#if defined(VERBOSE) || defined(KODO_VERBOSE)
        uint32_t rank_before = encoder->rank();
#endif
        encoder->set_symbol(encoder->rank(), {buf->head, (uint32_t)buf->len});
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Adding pkt of %lu bytes to encoder, rank before: %u, rank after: %u\n", buf->len, rank_before, encoder->rank());
#endif

        buf->clear();

        if (encoder->rank() * tx_redundancy > tx_delivered_pkts)
        {
            uint32_t amount_of_pkts = (encoder->rank() * tx_redundancy) - tx_delivered_pkts;
            if (encoder->rank() == tx_gen_size)
            {
                amount_of_pkts = std::ceil(encoder->rank() * tx_redundancy) - tx_delivered_pkts;
            }
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Sending TX generation, gen=%u, pkts=%u\n", tx_gen, amount_of_pkts);
#endif
            tx_delivered_pkts += amount_of_pkts;
            for (uint32_t i = 0; i < amount_of_pkts; ++i)
            {
                size_t len = encoder->encode(buf->head);
                buf->data_push(len);
                buf->data_len = tx_symbol_size;

#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Size of KODO (encoded) symbol %lu bytes, gen=%u\n", len, tx_gen);
#endif
                set_packet_symbol_size(buf, tx_symbol_size);
                set_packet_gen_size(buf, tx_gen_size);
                set_packet_gen(buf, tx_gen);
                set_packet_field<uint32_t>(buf, encoder->rank());

                super::read_pkt(buf);
                buf->clear();
            }

            if (encoder->rank() == tx_gen_size)
            {
                tx_new_generation();
            }

            return tx_symbol_size;
        }

        return 0;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            uint32_t check_gen = first_generation_with_undelivered_symbols();
            if (check_gen > 0)
            {
                return rx_deliver_next_symbol(buf, check_gen);
            }
        }
        WRITE(super, buf);

        update_rx_pkt_timeout();

        uint32_t pkt_encoder_rank = get_packet_field<uint32_t>(buf);
        uint32_t pkt_gen = get_packet_gen(buf);
        last_gen = pkt_gen;
        uint32_t pkt_gen_size = get_packet_gen_size(buf);
        uint32_t pkt_symbol_size = get_packet_symbol_size(buf);

        if (delivered_generations.count(pkt_gen) != 0)
        {
            // The generation has already been fully delivered.
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Skipping pkt of %lu bytes from gen %u, all symbols delivered\n", buf->len, pkt_gen);
#endif
            return 0;
        }

        if (decoders.count(pkt_gen) == 0)
        {
            // The generation has not been seen before, start it.
            rx_new_generation(pkt_gen, pkt_gen_size, pkt_symbol_size);
        }

        decoder_info* d = &decoders[pkt_gen]; // Convenience pointer

        ASSERT_EQ(delivered_generations.count(pkt_gen), 0);
        ASSERT_NEQ(decoders.count(pkt_gen), 0);
        ASSERT_EQ(pkt_gen_size, d->gen_size);
        ASSERT_EQ(pkt_symbol_size, d->symbol_size);

        d->encoder_rank = std::max(d->encoder_rank, pkt_encoder_rank);
        ASSERT_NEQ(d->encoder_rank, 0);
        ASSERT_LTE(d->encoder_rank, d->gen_size);

        if (!d->decoder->is_complete())
        {
            d->last_pkt_time = time_stamp();
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            uint32_t rank_before = d->decoder->rank();
#endif
            d->decoder->decode(buf->head);
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Adding pkt of %lu bytes to decoder, rank before: %u, rank after: %u\n", buf->len, rank_before, d->decoder->rank());
            d->decoder->print_cached_symbol_coefficients(std::cout);
#endif
            buf->clear();

            if (!rx_is_partial_complete(pkt_gen))
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("No symbols ready, gen=%u, rank=%u, dec=%u, del=%lu\n", pkt_gen, d->decoder->rank(), decoders[pkt_gen].decoder->symbols_decoded(), decoders[pkt_gen].delivered_symbols.size());
#endif
                return 0;
            }
            else if (d->decoder->is_complete())
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Decoding complete, gen=%u, rank=%u\n", pkt_gen, d->decoder->rank());
#endif
            }
            else
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Partial decoding complete, gen=%u, rank=%u\n", pkt_gen, d->decoder->rank());
#endif
            }
            return rx_deliver_next_symbol(buf, pkt_gen);
        }
        else
        {
            buf->clear();
        }

        return 0;
    }

    uint32_t get_tx_gen_size() const
    {
        return tx_gen_size;
    }

    uint32_t get_rx_gen_size(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        return decoders[gen].gen_size;
    }

    uint32_t get_tx_symbol_size() const
    {
        return tx_symbol_size-sizeof(T_zeropad);
    }

    uint32_t get_rx_symbol_size(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        return decoders[gen].symbol_size - sizeof(T_zeropad);
    }

    uint32_t get_rx_delivered_symbols(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        return decoders[gen].delivered_symbols.size();
    }

    uint32_t get_rx_gen() const
    {
        if (decoders.size() == 0)
        {
            return 0;
        }
        return decoders.crbegin()->first;
    }

    void failure()
    {
        decoders[last_gen].decoder->print_decoder_state(std::cout);
    }

    void print_decode_stat(std::fstream& fio)
    {
        ASSERT_EQ(fio.is_open(), true);
        fio << rx_seen_pkts << "," << rx_delivered_pkts << "," << (rx_seen_pkts == 0 ? 1.0 : rx_delivered_pkts/(double)rx_seen_pkts) << ",";
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
            total_inter_pkt_delay += std::min(std::max(i, 20UL), 500UL);
        }
        rx_pkt_timeout =
            total_inter_pkt_delay/((double)max_inter_pkt_delays*1000000);
        rx_pkt_timeout *= 1.05;
    }

    void tx_new_generation()
    {
        ++tx_gen;
        encoder_factory.set_symbols(tx_gen_size);
        encoder_factory.set_symbol_size(tx_symbol_size);
        encoder = encoder_factory.build();
//        encoder->seed(0);
        tx_delivered_pkts = 0;
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("New TX generation, gen=%u\n", tx_gen);
#endif
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

        rx_seen_pkts += gen_size;

#if defined(VERBOSE) || defined(KODO_VERBOSE)
        uint32_t last_rank = 0;
        uint32_t delivered_symbols = 0;
        if (decoders.size() != 0)
        {
            last_rank = decoders.crbegin()->second.decoder->rank();
            delivered_symbols =
                decoders.crbegin()->second.delivered_symbols.size();
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

    int rx_deliver_symbol(p_pkt_buffer& buf, uint32_t gen, uint32_t symbol)
    {
        ASSERT_EQ(buf->len, 0);
        decoders[gen].decoder->copy_symbol(symbol, {buf->head, decoders[gen].symbol_size});
        uint32_t zeros = get_packet_zeros(buf);
        buf->data_push(decoders[gen].symbol_size - zeros);
        decoders[gen].delivered_symbols[symbol] = true;
        ++rx_delivered_pkts;
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Delivering symbol %u from gen %u (%lu bytes), removed %u bytes zeropadding\n",
               symbol, gen, buf->len, zeros);
#endif
        if (decoders[gen].delivered_symbols.size() == decoders[gen].gen_size)
        {
            delivered_generations[gen] = true;
            while (delivered_generations.size() >
                   max_amount_of_delivered_generations)
            {
                delivered_generations.erase(delivered_generations.begin());
            }
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Erasing gen %u due to all symbols delivered\n", gen);
#endif
            decoders.erase(gen);
        }
        return buf->len;
    }

    // perhaps rename to 'rx_deliver_next_ready_symbol'
    int rx_deliver_next_symbol(p_pkt_buffer& buf, uint32_t gen)
    {
        for (uint32_t i = 0; i < decoders[gen].gen_size; ++i)
        {
            if (decoders[gen].delivered_symbols.count(i) == 0 &&
                decoders[gen].decoder->is_symbol_decoded(i))
            {
                return rx_deliver_symbol(buf, gen, i);
            }
        }
        return 0;
    }

    uint32_t first_generation_with_undelivered_symbols()
    {
        bool multiple_decoders = decoders.size() > 1;
        uint64_t rx_pkt_timeout_us = rx_pkt_timeout * 1000000 * 1000; // DEBUG!!
        uint64_t now = time_stamp();
        for (auto i = decoders.begin(); i != decoders.end(); /* NO INCREMENT */)
        {
            uint64_t time_out_time_first = rx_pkt_timeout_us + i->second.last_pkt_time;
            uint64_t time_out_time_last = rx_pkt_timeout_us * i->second.gen_size + i->second.first_pkt_time;
            bool timed_out = time_out_time_last < now && time_out_time_first < now;
            bool partial_complete = rx_is_partial_complete(i->first);
            if (partial_complete)
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("First gen is %u, last is %u\n",
                       i->first, decoders.crbegin()->first);
#endif
                return i->first;
            }
            else if (timed_out && multiple_decoders)
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                if (i->second.delivered_symbols.size() < i->second.gen_size)
                {
                    printf("Erasing gen %u due to time out, TTL %lu, del=%lu/%u/%u, gen was %scomplete\n",
                           i->first, now-i->second.first_pkt_time,
                           i->second.delivered_symbols.size(),
                           i->second.decoder->symbols_decoded(),
                           i->second.decoder->rank(),
                           (i->second.decoder->is_complete() ? "" : "NOT "));
                }
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

    bool rx_is_partial_complete(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return false;
        }
        return (decoders[gen].decoder->symbols_decoded() >
                decoders[gen].delivered_symbols.size());
    }
};
