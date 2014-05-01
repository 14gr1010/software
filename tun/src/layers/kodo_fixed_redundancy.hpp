#pragma once

#include <map> // std::map
#include <vector> // std::vector
#include <kodo/rlnc/full_vector_codes.hpp>

#include "kodo_redundancy.hpp"

template<class super>
class kodo_fixed_redundancy : public super
{
    typedef kodo::full_rlnc_encoder<fifi::binary8> rlnc_encoder;
    typedef kodo::debug_full_rlnc_decoder<fifi::binary8> rlnc_decoder;

    const int max_inter_pkt_delays = 15;

    struct decoder_info
    {
        uint64_t last_pkt_time = 0;
        uint64_t first_pkt_time = 0;
        rlnc_decoder::pointer decoder;
        uint32_t symbol_size = 0;
        uint32_t gen_size = 0;
        std::map<uint32_t, bool> delivered_symbols;
    };

    // Transmitter
    uint32_t tx_gen = 0;
    uint32_t tx_symbol_size = 1354 + sizeof(T_zeropad);
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
    bool rx_force_delivery = false;
    uint32_t rx_force_delivery_gen = 0;
    bool print = true;
    std::vector<uint64_t> inter_pkt_delays;
    int inter_pkt_delays_index;

public:
    kodo_fixed_redundancy(T_arg_list args = {})
    : super(args),
      encoder_factory(tx_max_gen_size, tx_symbol_size),
      decoder_factory(rx_max_gen_size, rx_max_symbol_size)
    {
        parse(args, false);
        tx_new_generation();
        inter_pkt_delays.resize(max_inter_pkt_delays);
        inter_pkt_delays_index = 0;
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
        tx_gen_size = get_arg("kodo::tx_gen_size", tx_gen_size, args).u32;
        ASSERT_LTE(tx_gen_size, tx_max_gen_size);
        uint32_t symbol_size = tx_symbol_size - sizeof(T_zeropad);
        tx_symbol_size = get_arg("kodo::tx_symbol_size", symbol_size, args).u32 + sizeof(T_zeropad);
    }

    virtual int read_pkt(p_pkt_buffer& buf)
    {
        ASSERT_LTE(buf->len, tx_symbol_size-sizeof(T_zeropad));

        T_zeropad zeros = buf->zero_pad_to(tx_symbol_size-sizeof(T_zeropad));
        set_packet_zeros(buf, zeros);

#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Adding %hu bytes of zeropadding\n", zeros);
#endif

#if defined(VERBOSE) || defined(KODO_VERBOSE)
        uint32_t rank_before = encoder->rank();
#endif
        encoder->set_symbol(encoder->rank(), sak::const_storage(buf->head, buf->len));
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Adding pkt of %lu bytes to encoder, rank before: %u, rank after: %u\n", buf->len, rank_before, encoder->rank());
#endif

        buf->clear();

        if (encoder->rank() == tx_gen_size)
        {
            uint32_t amount_of_pkts = std::ceil(tx_gen_size * tx_redundancy);
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Sending TX generation, gen=%u, pkts=%u\n", tx_gen, amount_of_pkts);
#endif
            for (uint32_t i = 0; i < amount_of_pkts; ++i)
            {
                buf->clear();
                size_t len = encoder->encode(buf->head);
                buf->data_push(len);

#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Size of KODO (encoded) symbol %lu bytes, gen=%u\n", len, tx_gen);
#endif

                set_packet_symbol_size(buf, tx_symbol_size);
                set_packet_gen_size(buf, tx_gen_size);
                set_packet_gen(buf, tx_gen);

                super::read_pkt(buf);
            }

            tx_new_generation();

            return tx_gen_size * tx_symbol_size; // Amount of data transmitted
        }

        return 0;
    }

    virtual int write_pkt(p_pkt_buffer& buf)
    {
        if (buf->is_empty())
        {
            uint32_t check_gen = first_generation_with_undelivered_symbols();
            if (true == rx_force_delivery)
            {
                int n = rx_force_deliver_next_symbol(buf, rx_force_delivery_gen);
                if (n > 0)
                {
                    return n;
                }
            }
            if (rx_is_partial_complete(check_gen))
            {
                return rx_deliver_next_symbol(buf, check_gen);
            }
        }
        WRITE(super, buf);

        update_rx_pkt_timeout();

        uint32_t pkt_gen = get_packet_gen(buf);
        uint32_t pkt_gen_size = get_packet_gen_size(buf);
        uint32_t pkt_symbol_size = get_packet_symbol_size(buf);

        if (decoders.count(pkt_gen) == 0)
        {
            rx_new_generation(pkt_gen, pkt_gen_size, pkt_symbol_size);
        }

        decoder_info* d = &decoders[pkt_gen];

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
            print = true;
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
        return decoders[gen].symbol_size-sizeof(T_zeropad);
    }

    uint32_t get_rx_delivered_symbols(uint32_t gen)
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        return decoders[gen].delivered_symbols.size();
    }

    uint32_t get_rx_rank(uint32_t gen) const
    {
        if (decoders.count(gen) == 0)
        {
            return 0;
        }
        uint32_t rank = decoders[gen].decoder->rank();
        return rank;
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
        inter_pkt_delays_index = (inter_pkt_delays_index + 1) % max_inter_pkt_delays;

        uint64_t total_inter_pkt_delay = 0;
        for (uint64_t i : inter_pkt_delays)
        {
            total_inter_pkt_delay += std::max(i, (uint64_t)20);
        }
        rx_pkt_timeout = total_inter_pkt_delay/((double)max_inter_pkt_delays*1000000);
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
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        uint32_t last_rank = 0;
        uint32_t delivered_symbols = 0;
        if (decoders.size() != 0)
        {
            last_rank = decoders.crbegin()->second.decoder->rank();
            delivered_symbols = decoders.crbegin()->second.delivered_symbols.size();
        }
        printf("RX gen=%u gs=%u ss=%u last gen: rank=%u del=%u\n", gen, gen_size, symbol_size, last_rank, delivered_symbols);
#endif
        decoders[gen] = d;
    }

    int rx_deliver_symbol(p_pkt_buffer& buf, uint32_t gen, uint32_t symbol)
    {
        ASSERT_EQ(buf->len, 0);
        decoders[gen].decoder->copy_symbol(symbol, {buf->head, decoders[gen].symbol_size});
        uint32_t zeros = get_packet_zeros(buf);
        buf->data_push(decoders[gen].symbol_size - zeros);
        decoders[gen].delivered_symbols[symbol] = true;
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("Delivering symbol %u from gen %u (%lu bytes)\n", symbol, gen, buf->len);
#endif
        return buf->len;
    }

    int rx_deliver_next_symbol(p_pkt_buffer& buf, uint32_t gen)
    {
        uint32_t delivered_symbols = decoders[gen].delivered_symbols.size();
        if (decoders[gen].decoder->is_symbol_decoded(delivered_symbols))
        {
            ASSERT_EQ(decoders[gen].delivered_symbols.count(delivered_symbols), 0);
            rx_deliver_symbol(buf, gen, delivered_symbols);

            print = true;

            return buf->len;
        }
        else if (print)
        {
            print = false;
            // No symbol delivered, to perserve order
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Symbol %u from gen %u NOT ready for delivery\n", delivered_symbols, gen);
#endif
        }
        return 0;
    }

    int rx_force_deliver_next_symbol(p_pkt_buffer& buf, uint32_t gen)
    {
        for (uint32_t i = 0; i < decoders[gen].gen_size; ++i)
        {
            if (decoders[gen].delivered_symbols.count(i) == 0 && decoders[gen].decoder->is_symbol_decoded(i))
            {
                return rx_deliver_symbol(buf, gen, i);
            }
        }
        rx_force_delivery = false;
        return 0;
    }

    uint32_t first_generation_with_undelivered_symbols()
    {
        if (true == rx_force_delivery)
        {
            return 0;
        }

        uint64_t now = time_stamp();
        for (auto i = decoders.begin(); i != decoders.end(); /* NO INCREMENT */)
        {
            uint64_t time_out_time_first = rx_pkt_timeout * 1000000 + i->second.last_pkt_time;
            uint64_t time_out_time_last = rx_pkt_timeout * i->second.gen_size * 1000000 + i->second.first_pkt_time;
            bool timed_out = time_out_time_last < now && time_out_time_first < now;
            bool partial_complete = rx_is_partial_complete(i->first);
            bool multiple_decoders = decoders.size() > 1;
            if (partial_complete && !timed_out)
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("First gen is %u, last is %u\n", i->first, decoders.crbegin()->first);
#endif
                return i->first;
            }
            else if (partial_complete && multiple_decoders)
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Gen %u timed out, del=%lu/%u/%u, forcing delivery of rest\n", i->first, i->second.delivered_symbols.size(), i->second.decoder->symbols_decoded(), i->second.decoder->rank());
#endif
                ASSERT_EQ(rx_force_delivery, false);
                rx_force_delivery = true;
                rx_force_delivery_gen = i->first;
                return 0;

            }
            else if (timed_out && multiple_decoders)
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                if (i->second.delivered_symbols.size() < i->second.gen_size)
                {
                    printf("Erasing gen %u due to time out, TTL %lu, del=%lu/%u/%u, gen was %scomplete\n", i->first, now-i->second.first_pkt_time, i->second.delivered_symbols.size(), i->second.decoder->symbols_decoded(), i->second.decoder->rank(), (i->second.decoder->is_complete() ? "" : "NOT "));
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
        return (decoders[gen].decoder->symbols_decoded() > decoders[gen].delivered_symbols.size());
    }
};
