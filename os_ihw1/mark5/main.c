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

__attribute__((noreturn)) static void exit_with_failure_message(
    const char* failure_message) {
    fprintf(stderr, "[>>>] Error: %s\n", failure_message);
    exit(EXIT_FAILURE);
}

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

static bool create_file_reader_impl(int read_file_fd, int reader_to_proc_fd) {
    if (read_file_fd < 0) {
        print_failure_message("File reader child: can't open input file");
        return false;
    }
    if (reader_to_proc_fd < 0) {
        print_failure_message(
            "File reader child: can't open fifo [file reader] -> [text processor]");
        return false;
    }

    char read_buffer[BUFFER_SIZE];
    ssize_t nbytes_read = read(read_file_fd, read_buffer, BUFFER_SIZE - 1);
    if (nbytes_read < 0) {
        print_failure_message("File reader child: can't read from the input file");
        return false;
    }

    if (write(reader_to_proc_fd, read_buffer, (size_t)nbytes_read) < 0) {
        print_failure_message(
            "File reader child: can't send data to another process using fifo [file "
            "reader] -> [text processor]");
        return false;
    }

    return true;
}

static pid_t create_file_reader(const char* input_filename) {
    pid_t child_pid = fork();
    if (child_pid != 0) {
        return child_pid;
    }

    int read_file_fd      = open(input_filename, O_RDONLY);
    int reader_to_proc_fd = open(READER_TO_PROC_FIFO, O_WRONLY);
    bool result           = create_file_reader_impl(read_file_fd, reader_to_proc_fd);
    if (read_file_fd >= 0) {
        if (close(read_file_fd) < 0) {
            print_failure_message("File reader child: can't close opened input file");
            result = false;
        }
    }
    if (reader_to_proc_fd >= 0) {
        if (close(reader_to_proc_fd) < 0) {
            print_failure_message(
                "File reader child: can't close writing side of the fifo [file reader] "
                "-> "
                "[text processor]");
            result = false;
        }
    }

    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool create_data_processor_impl(uint32_t start_n, uint32_t end_n,
                                       int reader_to_proc_fifo_fd,
                                       int proc_to_writer_fd) {
    if (reader_to_proc_fifo_fd < 0) {
        print_failure_message(
            "Data processor child: can't open fifo [file reader] -> [text processor]");
        return false;
    }
    if (proc_to_writer_fd < 0) {
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
    if (write(proc_to_writer_fd, read_buffer, string_length) < 0) {
        print_failure_message(
            "Data processor child: can't send data to another process using fifo [text "
            "processor] -> [file writer]");
        return false;
    }

    return true;
}

static pid_t create_data_processor(uint32_t start_n, uint32_t end_n) {
    assert(start_n <= end_n);
    pid_t child_pid = fork();
    if (child_pid != 0) {
        return child_pid;
    }

    int reader_to_proc_fifo_fd = open(READER_TO_PROC_FIFO, O_RDONLY);
    int proc_to_writer_fd      = open(PROC_TO_WRITER_FIFO, O_WRONLY);
    bool result = create_data_processor_impl(start_n, end_n, reader_to_proc_fifo_fd,
                                             proc_to_writer_fd);
    if (reader_to_proc_fifo_fd >= 0) {
        if (close(reader_to_proc_fifo_fd) < 0) {
            print_failure_message(
                "Data processor child: can't open close opened fifo  [file reader] -> "
                "[text "
                "processor]");
            result = false;
        }
    }
    if (proc_to_writer_fd >= 0) {
        if (close(proc_to_writer_fd) < 0) {
            print_failure_message(
                "Data processor child: can't open close opened fifo [text processor] -> "
                "[file writer]");
            result = false;
        }
    }

    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool create_file_writer_impl(int write_file_fd, int processor_to_writer_fd) {
    if (write_file_fd < 0) {
        print_failure_message("File writer child: can't open output file");
        return false;
    }
    if (processor_to_writer_fd < 0) {
        print_failure_message(
            "File writer child: can't open fifo [text processor] -> [file writer]");
        return false;
    }

    char read_buffer[BUFFER_SIZE];
    ssize_t nbytes_read = read(processor_to_writer_fd, read_buffer, BUFFER_SIZE - 1);
    if (nbytes_read < 0) {
        print_failure_message(
            "File writer child: can't read from the pipe [text processor] -> [file "
            "writer]");
        return false;
    }

    if (write(write_file_fd, read_buffer, (size_t)nbytes_read) < 0) {
        print_failure_message("File writer child: can't write to output file");
        return false;
    }

    return true;
}

static pid_t create_file_writer(const char* output_filename) {
    pid_t child_pid = fork();
    if (child_pid != 0) {
        return child_pid;
    }

    int write_file_fd          = open(output_filename, O_WRONLY | O_CREAT, 0644);
    int processor_to_writer_fd = open(PROC_TO_WRITER_FIFO, O_RDONLY);
    bool result = create_file_writer_impl(write_file_fd, processor_to_writer_fd);
    if (write_file_fd >= 0) {
        if (close(write_file_fd) < 0) {
            print_failure_message("File writer child: can't close opened output file");
            result = false;
        }
    }
    if (processor_to_writer_fd >= 0) {
        if (close(processor_to_writer_fd) < 0) {
            print_failure_message(
                "File writer child: can't close opened fifo [processor] -> [file "
                "writer]");
            result = false;
        }
    }

    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool run(parse_result_t parse_result) {
    umask(0);
    mknod(READER_TO_PROC_FIFO, S_IFIFO | 0666, 0);
    umask(0);
    mknod(PROC_TO_WRITER_FIFO, S_IFIFO | 0666, 0);

    pid_t file_reader_pid = create_file_reader(parse_result.input_filename);
    if (file_reader_pid == -1) {
        print_failure_message(
            "Main process: Can't create child process for reading file");
        return false;
    }

    pid_t data_processor_pid =
        create_data_processor(parse_result.start_n1, parse_result.end_n2);
    if (data_processor_pid == -1) {
        print_failure_message(
            "Main process: Can't create child process for processing data");
        return false;
    }

    pid_t file_writer_pid = create_file_writer(parse_result.output_filename);
    if (file_writer_pid == -1) {
        print_failure_message(
            "Main process: Can't create child process for writing file");
        return false;
    }

    int status;
    waitpid(file_reader_pid, &status, 0);
    if (status != EXIT_SUCCESS) {
        return false;
    }
    waitpid(data_processor_pid, &status, 0);
    if (status != EXIT_SUCCESS) {
        return false;
    }
    waitpid(file_writer_pid, &status, 0);
    if (status != EXIT_SUCCESS) {
        return false;
    }

    return true;
}

int main(int argc, const char* const argv[]) {
    parse_result_t parse_result = parse_arguments(argc, argv);
    if (parse_result.status != PARSE_OK) {
        handle_parsing_error(parse_result.status);
        exit_with_failure_message("Incorrect arguments provided");
    }

    bool result = run(parse_result);
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
