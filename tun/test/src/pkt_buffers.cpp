#include "pkt_buffers.hpp"

#include "../test_case.hpp"

#include "layers/xor_adaptive_redundancy.hpp"
#include "layers/data_pointer.hpp"
#include "layers/final_layer.hpp"

void xor_buffers_1()
{
    INIT_TEST_CASE;

    size_t size = 61;
    int r = -1;

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();
    p_pkt_buffer C = new_pkt_buffer();
    p_pkt_buffer D = new_pkt_buffer();

    A->data_push(size);
    B->data_push(size);
    C->data_push(size);
    D->data_push(size);

    memset(A->head, 0xFF, A->len);
    memset(B->head, 0xCC, B->len);
    memset(C->head, 0x33, C->len);
    memset(D->head, 0xFF, D->len);

    A->xor_in(B);
    r = memcmp(A->head, C->head, size);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);

    A->xor_in(B);
    r = memcmp(A->head, D->head, size);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        D->print_head(D->len, "D");
    }
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void xor_buffers_2()
{
    INIT_TEST_CASE;

    size_t size_1 = 61;
    size_t size_2 = 31;
    int r = -1;

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();
    p_pkt_buffer C = new_pkt_buffer();
    p_pkt_buffer D = new_pkt_buffer();
    p_pkt_buffer E = new_pkt_buffer();

    A->data_push(size_2);
    B->data_push(size_1);
    C->data_push(size_2);
    D->data_push(size_2);
    E->data_push(size_2);

    memset(A->head, 0xFF, A->len);
    memset(B->head, 0xCC, B->len);
    memset(C->head, 0x33, C->len);
    memset(D->head, 0x00, D->len);
    memset(E->head, 0xFF, E->len);

    TEST_EQ(A->len, size_2);

    A->xor_in(B);

    TEST_EQ(A->len, size_1);

    r = memcmp(A->head, C->head, size_2);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);
    r = memcmp(A->head+size_2, B->head, (size_1 - size_2));
    if (r != 0)
    {
        A->print_head(A->len, "A");
        B->print_head(B->len, "B");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);

    A->xor_in(B);

    TEST_EQ(A->len, size_1);

    r = memcmp(A->head, E->head, size_2);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        E->print_head(E->len, "E");
    }
    TEST_EQ(r, 0);
    r = memcmp(A->head+size_2, D->head, (size_1 - size_2));
    if (r != 0)
    {
        A->print_head(A->len, "A");
        D->print_head(D->len, "D");
        E->print_head(E->len, "E");
    }
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void xor_buffers_3()
{
    INIT_TEST_CASE;

    size_t size_1 = 61;
    size_t size_2 = 31;
    int r = -1;

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();
    p_pkt_buffer C = new_pkt_buffer();
    p_pkt_buffer D = new_pkt_buffer();

    A->data_push(size_1);
    B->data_push(size_2);
    C->data_push(size_2);
    D->data_push(size_1);

    memset(A->head, 0xFF, A->len);
    memset(B->head, 0xCC, B->len);
    memset(C->head, 0x33, C->len);
    memset(D->head, 0xFF, D->len);

    TEST_EQ(A->len, size_1);

    A->xor_in(B);

    TEST_EQ(A->len, size_1);

    r = memcmp(A->head, C->head, size_2);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);
    r = memcmp(A->head+size_2, D->head, (size_1 - size_2));
    if (r != 0)
    {
        A->print_head(A->len, "A");
        C->print_head(C->len, "C");
        D->print_head(D->len, "D");
    }
    TEST_EQ(r, 0);

    A->xor_in(B);
    r = memcmp(A->head, D->head, size_1);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        D->print_head(D->len, "D");
    }
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void xor_buffers_4()
{
    INIT_TEST_CASE;

    size_t size_1 = 61;
    size_t size_2 = 31;
    int r = -1;

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();
    p_pkt_buffer C = new_pkt_buffer();
    p_pkt_buffer D = new_pkt_buffer();

    A->data_push(size_2);
    B->data_push(size_1);
    C->data_push(size_2);
    D->data_push(size_1);

    memset(A->head, 0xFF, A->len);
    memset(B->head, 0xCC, B->len);
    memset(C->head, 0x33, C->len);
    memset(D->head, 0xFF, D->len);

    TEST_EQ(A->len, size_2);

    A->xor_in(B);

    TEST_EQ(A->len, size_1);

    r = memcmp(A->head, C->head, size_2);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);
    r = memcmp(A->head+size_2, B->head, (size_1 - size_2));
    if (r != 0)
    {
        A->print_head(A->len, "A");
        B->print_head(B->len, "B");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);

    A->xor_out(B, size_2);

    TEST_EQ(A->len, size_2);

    r = memcmp(A->head, D->head, size_2);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        D->print_head(D->len, "D");
    }
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void zero_pad()
{
    INIT_TEST_CASE;

    size_t size_1 = 31;
    size_t size_2 = 61;
    int r = -1;
    size_t zeros = -1;

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();
    p_pkt_buffer C = new_pkt_buffer();

    A->data_push(size_1);
    B->data_push(size_1);
    C->data_push(size_2 - size_1);

    memset(A->head, 0xFF, A->len);
    memset(B->head, 0xFF, B->len);
    memset(C->head, 0x00, C->len);

    TEST_EQ(A->len, size_1);

    zeros = A->zero_pad_to(size_2);

    TEST_EQ(zeros, size_2 - size_1);

    TEST_EQ(A->len, size_2);

    r = memcmp(A->head, B->head, size_1);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        B->print_head(B->len, "B");
    }
    TEST_EQ(r, 0);

    r = memcmp(A->head + size_1, C->head, (size_2 - size_1));
    if (r != 0)
    {
        A->print_head(A->len, "A");
        B->print_head(B->len, "B");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);

    CONCLUDE_TEST_CASE;
}

void xor_redundancy_info()
{
    INIT_TEST_CASE;

    p_pkt_buffer buf = new_pkt_buffer();

    T_pkt_id id = 1;
    T_pkt_redundancy type = SYSTEMATIC_PKT;
    T_pkt_len len = 3;
    uint8_t xors = 5;
    uint8_t u8 = 255;
    uint16_t u16 = 300;
    uint32_t u32 = 1234567;
    uint64_t u64 = 1234567890;
    int8_t i8 = -120;
    int16_t i16 = -300;
    int32_t i32 = -123456;
    int64_t i64 = -12345678;

    set_packet_id(buf, id);
    set_packet_type(buf, type);
    set_packet_len(buf, len);
    set_packet_xors(buf, xors);
    set_packet_field<uint8_t>(buf, u8);
    set_packet_field<uint16_t>(buf, u16);
    set_packet_field<uint32_t>(buf, u32);
    set_packet_field<uint64_t>(buf, u64);
    set_packet_field<int8_t>(buf, i8);
    set_packet_field<int16_t>(buf, i16);
    set_packet_field<int32_t>(buf, i32);
    set_packet_field<int64_t>(buf, i64);

    TEST_EQ(i64,  read_packet_field<int64_t>(buf));
    TEST_EQ(i64,  get_packet_field<int64_t>(buf));
    TEST_EQ(i32,  read_packet_field<int32_t>(buf));
    TEST_EQ(i32,  get_packet_field<int32_t>(buf));
    TEST_EQ(i16,  read_packet_field<int16_t>(buf));
    TEST_EQ(i16,  get_packet_field<int16_t>(buf));
    TEST_EQ(i8,   read_packet_field<int8_t>(buf));
    TEST_EQ(i8,   get_packet_field<int8_t>(buf));
    TEST_EQ(u64,  read_packet_field<uint64_t>(buf));
    TEST_EQ(u64,  get_packet_field<uint64_t>(buf));
    TEST_EQ(u32,  read_packet_field<uint32_t>(buf));
    TEST_EQ(u32,  get_packet_field<uint32_t>(buf));
    TEST_EQ(u16,  read_packet_field<uint16_t>(buf));
    TEST_EQ(u16,  get_packet_field<uint16_t>(buf));
    TEST_EQ(u8,   read_packet_field<uint8_t>(buf));
    TEST_EQ(u8,   get_packet_field<uint8_t>(buf));
    TEST_EQ(xors, get_packet_xors(buf));
    TEST_EQ(len,  get_packet_len(buf));
    TEST_EQ(type, get_packet_type(buf));
    TEST_EQ(id,   get_packet_id(buf));

    CONCLUDE_TEST_CASE;
}

void data_pointer_1()
{
    INIT_TEST_CASE;

    uint32_t data_len = 13;
    uint32_t hdr_len = 7;
    int r = -1;

    data_pointer<final_layer>* d = new data_pointer<final_layer>();

    p_pkt_buffer A = new_pkt_buffer();
    p_pkt_buffer B = new_pkt_buffer();
    p_pkt_buffer C = new_pkt_buffer();

    A->data_push(data_len);
    A->head_push(hdr_len);
    B->head_push(data_len);
    C->head_push(hdr_len);

    memset(A->data, 0xFF, data_len);
    memset(A->head, 0x44, hdr_len);
    memset(B->head, 0xFF, data_len);
    memset(C->head, 0x44, hdr_len);

    TEST_EQ(A->len, data_len + hdr_len);

    d->read_pkt(A);

    TEST_EQ(A->len, data_len + hdr_len + sizeof(T_hdr_len));

    A->data = A->head;

    d->write_pkt(A);

    TEST_EQ(A->len, data_len + hdr_len);
    TEST_EQ(A->data - A->head, hdr_len);

    r = memcmp(A->head, C->head, hdr_len);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        B->print_head(B->len, "B");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);

    r = memcmp(A->data, B->head, data_len);
    if (r != 0)
    {
        A->print_head(A->len, "A");
        B->print_head(B->len, "B");
        C->print_head(C->len, "C");
    }
    TEST_EQ(r, 0);

    delete d;

    CONCLUDE_TEST_CASE;
}
