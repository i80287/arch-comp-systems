#define _XOPEN_SOURCE 700

#include <assert.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"

static const size_t TRANSFER_BUFFER_SIZE      = 128 * sizeof(char);
static const char END_OF_COMMUNICATION_CHAR   = (char)128;
static const char READER_TO_PROC_QUEUE_NAME[] = "/reader-to-proc-q";
static const char PROC_TO_WRITER_QUEUE_NAME[] = "/proc-to-writer-q";

static void print_failure_message(const char* failure_message) {
    perror(failure_message);
    fprintf(stderr, "[>>>] Error: %s\n", failure_message);
}

static void swap(unsigned char* ustring, size_t l, size_t r) {
    unsigned char cl = ustring[l];
    ustring[l]       = ustring[r];
    ustring[r]       = cl;
}

static void reverse_substring(char* string, size_t l, size_t r) {
    assert(l <= r);
    unsigned char* ustring = (unsigned char*)string;
    while (l < r) {
        swap(ustring, l, r);
        l++;
        r--;
    }
}

static size_t min(size_t a, size_t b) { return a <= b ? a : b; }

static size_t max(size_t a, size_t b) { return a >= b ? a : b; }

static bool run_impl(uint32_t start_n, uint32_t end_n, mqd_t reader_to_proc_q,
                     mqd_t processor_to_writer_q) {
    if (reader_to_proc_q == -1) {
        print_failure_message(
            "Data processor: can't open queue [file reader] -> [text processor]");
        return false;
    }
    if (processor_to_writer_q == -1) {
        print_failure_message(
            "Data processor: can't open queue [text processor] -> [file writer]");
        return false;
    }

    for (size_t current_index = 0;;) {
        char read_buffer[TRANSFER_BUFFER_SIZE];
        struct mq_attr qs;
        mq_getattr(reader_to_proc_q, &qs);
        printf("%ld\n", qs.mq_msgsize);
        ssize_t nbytes_read =
            mq_receive(reader_to_proc_q, read_buffer, TRANSFER_BUFFER_SIZE - 1, 0);
        if (nbytes_read < 0) {
            print_failure_message(
                "Data processor: can't read from queue [file reader] -> [text "
                "processor]");
            return false;
        }
        if (nbytes_read == 0) {
            break;
        }

        size_t string_length = (size_t)nbytes_read;
        bool end_of_communication =
            read_buffer[string_length] == END_OF_COMMUNICATION_CHAR;
        if (end_of_communication) {
            break;
        }

        if (current_index <= start_n && start_n < current_index + string_length) {
            size_t l = start_n - current_index;
            size_t r = min(end_n, current_index + string_length - 1);
            reverse_substring(read_buffer, l, r);
        } else if (current_index <= end_n && end_n < current_index + string_length) {
            size_t l = max(start_n, current_index) - current_index;
            size_t r = end_n - current_index;
            reverse_substring(read_buffer, l, r);
        }
        current_index += string_length;
        if (mq_send(processor_to_writer_q, read_buffer, string_length, 0) < 0) {
            print_failure_message(
                "Data processor: can't send data to another process using queue [text "
                "processor] -> [file writer]");
            return false;
        }
    }

    return true;
}

static bool run(parse_result_t parse_result) {
    struct mq_attr mqattrs = {0};
    mqattrs.mq_maxmsg      = 10;
    mqattrs.mq_msgsize     = (long)(TRANSFER_BUFFER_SIZE - 1);
    mqd_t reader_to_proc_q = mq_open(READER_TO_PROC_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &mqattrs);
    mqd_t processor_to_writer_q = mq_open(PROC_TO_WRITER_QUEUE_NAME, O_WRONLY | O_CREAT, 0666, &mqattrs);

    uint32_t start_n = parse_result.start_n1;
    uint32_t end_n   = parse_result.end_n2;
    assert(start_n <= end_n);
    bool result = run_impl(start_n, end_n, reader_to_proc_q, processor_to_writer_q);
    if (reader_to_proc_q != -1) {
        if (mq_close(reader_to_proc_q) == -1) {
            print_failure_message(
                "Data processor: can't open close queue [file reader] -> [text "
                "processor]");
            result = false;
        }
    }
    if (processor_to_writer_q != -1) {
        if (mq_close(processor_to_writer_q) == -1) {
            print_failure_message(
                "Data processor: can't open close queue [text processor] -> [file "
                "writer]");
            result = false;
        }
    }

    return result;
}

int main(int argc, const char* const argv[]) {
    parse_result_t parse_result = parse_arguments(argc, argv);
    if (parse_result.status != PARSE_OK) {
        handle_parsing_error(parse_result.status);
        print_failure_message("Incorrect arguments provided");
        return EXIT_FAILURE;
    }

    bool result = run(parse_result);
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
