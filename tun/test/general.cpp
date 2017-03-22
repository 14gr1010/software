#include "general.hpp"

void signal_int(int sig)
{
    if (sig == SIGSEGV)
    {
        std::cout << TERMINAL_RED << "Segmentation fault!" << TERMINAL_END << std::endl;
    }
    std::cout << std::endl;
    CONCLUDE_TEST_SUITE;
}

int main(int argc, char** argv)
{
    signal(SIGINT, signal_int);
    signal(SIGSEGV, signal_int);

    ADD_TEST_CASE(connector, create_connector_class_1);
    ADD_TEST_CASE(connector, create_connector_class_2);
    ADD_TEST_CASE(connector, create_connector_class_3);
    ADD_TEST_CASE(connector, create_connector_class_4);
    ADD_TEST_CASE(connector, create_connector_class_5);
    ADD_TEST_CASE(connector, create_connector_class_6);
    ADD_TEST_CASE(connector, create_connector_class_7);
    ADD_TEST_CASE(connector, create_recoder_class_1);
    ADD_TEST_CASE(connector, create_forwarder_class_1);
    ADD_TEST_CASE(connector, create_connector_multipath_class_1);

    ADD_TEST_CASE(stack_args, args_1);
    ADD_TEST_CASE(stack_args, args_2);

    ADD_TEST_CASE(pkt_buffers, xor_buffers_1);
    ADD_TEST_CASE(pkt_buffers, xor_buffers_2);
    ADD_TEST_CASE(pkt_buffers, xor_buffers_3);
    ADD_TEST_CASE(pkt_buffers, xor_buffers_4);
    ADD_TEST_CASE(pkt_buffers, xor_redundancy_info);
    ADD_TEST_CASE(pkt_buffers, zero_pad);
    ADD_TEST_CASE(pkt_buffers, data_pointer_1);

    ADD_TEST_CASE(drop, drop_rate);

    ADD_TEST_CASE(file, write_read_from_file);

    ADD_TEST_CASE(delay, delay_pkt_1);
    ADD_TEST_CASE(delay, delay_pkt_2);
    ADD_TEST_CASE(delay, delay_pkt_3);
    ADD_TEST_CASE(delay, delay_pkt_4);

    ADD_TEST_CASE(xor_redundancy, xor_redundancy_1);
    ADD_TEST_CASE(xor_redundancy, xor_redundancy_2);
    ADD_TEST_CASE(xor_redundancy, xor_redundancy_3);

//    ADD_TEST_CASE(kodo, kodo_1);
//    ADD_TEST_CASE(kodo, kodo_2);
//    ADD_TEST_CASE(kodo, kodo_3);
//    ADD_TEST_CASE(kodo, kodo_4);
//    ADD_TEST_CASE(kodo, kodo_5);
//    ADD_TEST_CASE(kodo, kodo_6);
//    ADD_TEST_CASE(kodo, kodo_7);
//    ADD_TEST_CASE(kodo, kodo_8);
//    ADD_TEST_CASE(kodo, kodo_9);

    ADD_TEST_CASE(filter, filter_1);
    ADD_TEST_CASE(filter, filter_2);
    ADD_TEST_CASE(filter, filter_3);

    ADD_TEST_CASE(generators, generator_toggle_1);
    ADD_TEST_CASE(generators, generator_toggle_2);
    ADD_TEST_CASE(generators, generator_random_1);
    ADD_TEST_CASE(generators, generator_random_2);

    RUN;

    CONCLUDE_TEST_SUITE;
}
