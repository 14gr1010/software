#pragma once

#include <kodo/rlnc/on_the_fly_codes.hpp>

#include "kodo_redundancy.hpp"

template<class super>
class kodo_on_the_fly_simple_redundancy : public super
{
    typedef kodo::on_the_fly_encoder<fifi::binary8> rlnc_encoder;
    typedef kodo::debug_on_the_fly_decoder<fifi::binary8> rlnc_decoder;

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
    uint32_t rx_gen = 0;
    uint32_t rx_symbol_size = 0;
    uint32_t rx_gen_size = 0;
    uint32_t rx_max_gen_size = 100;
    uint32_t rx_max_symbol_size = 1360;
    uint32_t rx_delivered_symbols = 0;
    rlnc_decoder::factory decoder_factory;
    rlnc_decoder::pointer decoder;

    bool print = true;

public:
    kodo_on_the_fly_simple_redundancy(T_arg_list args = {})
    : super(args),
      encoder_factory(tx_max_gen_size, tx_symbol_size),
      decoder_factory(rx_max_gen_size, rx_max_symbol_size)
    {
        parse(args, false);
        tx_new_generation();
    }

    virtual void parse(T_arg_list args, bool call_super = true)
    {
        if (call_super)
        {
            super::parse(args);
        }

        tx_redundancy = get_arg("kodo::tx_redundancy", tx_redundancy, args).d;
        ASSERT_GTE(tx_redundancy, 1);
        tx_gen_size = get_arg("kodo::tx_gen_size", tx_gen_size, args).u32;
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

#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Size of KODO (encoded) symbol %lu bytes, gen=%u\n", len, tx_gen);
#endif

                set_packet_symbol_size(buf, tx_symbol_size);
                set_packet_gen_size(buf, tx_gen_size);
                set_packet_gen(buf, tx_gen);

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
        if (rx_delivered_symbols != rx_gen_size &&
            buf->is_empty() &&
            rx_is_partial_complete())
        {
            rx_deliver_symbol(buf);
            return buf->len;
        }
        WRITE(super, buf);

        uint32_t pkt_gen = get_packet_gen(buf);
        uint32_t pkt_gen_size = get_packet_gen_size(buf);
        uint32_t pkt_symbol_size = get_packet_symbol_size(buf);

        if (pkt_gen < rx_gen)
        {
            buf->clear();
            return 0;
        }
        else if (pkt_gen > rx_gen)
        {
            rx_new_generation(pkt_gen, pkt_gen_size, pkt_symbol_size);
        }

        if (!decoder->is_complete())
        {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            uint32_t rank_before = decoder->rank();
#endif
            decoder->decode(buf->head);
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Adding pkt of %lu bytes to decoder, rank before: %u, rank after: %u\n", buf->len, rank_before, decoder->rank());
            decoder->print_cached_symbol_coefficients(std::cout);
#endif
            buf->clear();

            if (!rx_is_partial_complete())
            {
                return 0;
            }
            else if (decoder->is_complete())
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
             printf("Decoding complete, gen=%u\n", rx_gen);
#endif
            }
            else
            {
#if defined(VERBOSE) || defined(KODO_VERBOSE)
                printf("Partial decoding complete, gen=%u\n", rx_gen);
#endif
            }
            print = true;
            rx_deliver_symbol(buf);

            return buf->len;
        }

        return 0;
    }

    uint32_t get_tx_gen_size() const
    {
        return tx_gen_size;
    }

    uint32_t get_rx_gen_size() const
    {
        return rx_gen_size;
    }

    uint32_t get_tx_symbol_size() const
    {
        return tx_symbol_size-sizeof(T_zeropad);
    }

    uint32_t get_rx_symbol_size() const
    {
        return rx_symbol_size-sizeof(T_zeropad);
    }

    uint32_t get_rx_delivered_symbols() const
    {
        return rx_delivered_symbols;
    }

private:
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

        decoder_factory.set_symbols(gen_size);
        decoder_factory.set_symbol_size(symbol_size);
        decoder = decoder_factory.build();
        rx_gen = gen;
        rx_gen_size = gen_size;
        rx_symbol_size = symbol_size;
        rx_delivered_symbols = 0;
#if defined(VERBOSE) || defined(KODO_VERBOSE)
        printf("New RX generation, gen=%u, gen_size=%u, symbols_size=%u\n", rx_gen, rx_gen_size, rx_symbol_size);
#endif
    }

    void rx_deliver_symbol(p_pkt_buffer& buf)
    {
        buf->clear();
        if (decoder->is_symbol_decoded(rx_delivered_symbols))
        {
            decoder->copy_symbol(rx_delivered_symbols, {buf->head, rx_symbol_size});

            uint32_t zeros = get_packet_zeros(buf);
            buf->data_push(rx_symbol_size - zeros);
            ++rx_delivered_symbols;
            print = true;
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Delivering pkt of %lu bytes, delivered symbols: %u\n", buf->len, rx_delivered_symbols);
#endif
        }
        else if (print)
        {
            print = false;
            // No symbol delivered, to perserve order. This should be changed
#if defined(VERBOSE) || defined(KODO_VERBOSE)
            printf("Symbol: %u NOT ready for delivery\n", rx_delivered_symbols);
#endif
        }
    }

    bool rx_is_partial_complete()
    {
        return (decoder->symbols_decoded() > rx_delivered_symbols);
    }
};
