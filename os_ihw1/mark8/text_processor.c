#define _XOPEN_SOURCE 700

#include <assert.h>
#include <fcntl.h>
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

static const size_t BUFFER_SIZE         = 5000 * sizeof(char);
static const char READER_TO_PROC_FIFO[] = "reader-to-proc.fifo";
static const char PROC_TO_WRITER_FIFO[] = "proc-to-writer.fifo";

static void print_failure_message(const char* failure_message) {
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

static bool run_impl(uint32_t start_n, uint32_t end_n, int reader_to_proc_fifo_fd,
                     int proc_to_writer_fifo_fd) {
    if (reader_to_proc_fifo_fd < 0) {
        print_failure_message(
            "Data processor child: can't open fifo [file reader] -> [text processor]");
        return false;
    }
    if (proc_to_writer_fifo_fd < 0) {
        print_failure_message(
            "Data processor child: can't open fifo [text processor] -> [file writer]");
        return false;
    }

    char read_buffer[BUFFER_SIZE];
    ssize_t nbytes_read = read(reader_to_proc_fifo_fd, read_buffer, BUFFER_SIZE - 1);
    if (nbytes_read < 0) {
        print_failure_message(
            "Data processor child: can't read from fifo [file reader] -> [text "
            "processor]");
        return false;
    }

    if (nbytes_read == 0) {
        print_failure_message(
            "Data processor child: empty input from fifo [file reader] -> [text "
            "processor]");
        return false;
    }

    size_t string_length = (size_t)nbytes_read;
    if (end_n >= string_length) {
        print_failure_message(
            "Data processor child: overflow: end index >= string length");
        return false;
    }

    reverse_substring(read_buffer, start_n, end_n);
    if (write(proc_to_writer_fifo_fd, read_buffer, string_length) < 0) {
        print_failure_message(
            "Data processor child: can't send data to another process using fifo [text "
            "processor] -> [file writer]");
        return false;
    }

    return true;
}

static bool run(parse_result_t parse_result) {
    umask(0);
    mknod(READER_TO_PROC_FIFO, S_IFIFO | 0666, 0);
    umask(0);
    mknod(PROC_TO_WRITER_FIFO, S_IFIFO | 0666, 0);

    uint32_t start_n = parse_result.start_n1;
    uint32_t end_n   = parse_result.end_n2;
    assert(start_n <= end_n);
    int reader_to_proc_fifo_fd = open(READER_TO_PROC_FIFO, O_RDONLY);
    int proc_to_writer_fifo_fd = open(PROC_TO_WRITER_FIFO, O_WRONLY);
    bool result =
        run_impl(start_n, end_n, reader_to_proc_fifo_fd, proc_to_writer_fifo_fd);
    if (reader_to_proc_fifo_fd >= 0) {
        if (close(reader_to_proc_fifo_fd) < 0) {
            print_failure_message(
                "Data processor child: can't open close opened fifo  [file reader] -> "
                "[text "
                "processor]");
            result = false;
        }
    }
    if (proc_to_writer_fifo_fd >= 0) {
        if (close(proc_to_writer_fifo_fd) < 0) {
            print_failure_message(
                "Data processor child: can't open close opened fifo [text processor] -> "
                "[file writer]");
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
