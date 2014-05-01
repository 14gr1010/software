#include "file_layer.hpp"

#include <cstring>

#include "../test_case.hpp"

#include "layers/file.hpp"
#include "layers/fd_io.hpp"
#include "layers/final_layer.hpp"

void write_read_from_file()
{
    INIT_TEST_CASE;

    typedef file<
            fd_io<
            final_layer
            >> file_stack;

    size_t length = 101;

    p_pkt_buffer A = new_pkt_buffer();
    A->data_push(length);
    memset(A->head, 0x30, A->len);

    char file_name[] = "write_read_from_file_test.txt";
    {
        file_stack* f = new file_stack({ {"file::name", file_name},
                                         {"file::flags", O_WRONLY | O_TRUNC | O_CREAT},
                                         {"file::mode", S_IRUSR | S_IWUSR} });
        TEST_EQ(A->len, length);
        f->write_pkt(A);

        delete f;
    }
    A->clear();
    {
        file_stack* f = new file_stack({ {"file::name", file_name},
                                         {"file::flags", O_RDONLY} });
        f->read_pkt(A);
        TEST_EQ(A->len, length);
        
        delete f;
    }

    if (TEST_PASSED)
    {
        remove(file_name);
    }

    CONCLUDE_TEST_CASE;
}
