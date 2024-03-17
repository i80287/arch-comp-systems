#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"

static const size_t BUFFER_SIZE = 5000 * sizeof(char);
static const bool DEBUG_PRINT   = false;

__attribute__((noreturn)) static void exit_with_failure_message(
    const char* failure_message) {
    fprintf(stderr, "[>>>] Error: %s\n", failure_message);
    exit(EXIT_FAILURE);
}

static void debug_print(const char* s) {
    if (DEBUG_PRINT) {
        puts(s);
    }
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

static bool create_file_reader_writer_impl(int read_file_fd,
                                           int fd_reader_to_processor[2],
                                           int write_file_fd,
                                           int fd_processor_to_writer[2]) {
    if (close(fd_reader_to_processor[0]) < 0) {
        print_failure_message(
            "File reader child: can't close reading side of the pipe [file reader] -> "
            "[text processor]");
        return false;
    }
    debug_print("reader closed [reader] <- [processor]");
    fd_reader_to_processor[0] = -1;

    if (read_file_fd < 0) {
        print_failure_message("File reader child: can't open input file");
        return false;
    }

    if (close(fd_processor_to_writer[1]) < 0) {
        print_failure_message(
            "File writer child: can't close writing side of the pipe [text processor] -> "
            "[file writer]");
        return false;
    }
    debug_print("writer closed [processor] <- [writer]");
    fd_processor_to_writer[1] = -1;

    if (write_file_fd < 0) {
        print_failure_message("File writer child: can't open output file");
        return false;
    }

    char read_buffer[BUFFER_SIZE];
    ssize_t nbytes_read = read(read_file_fd, read_buffer, BUFFER_SIZE - 1);
    if (nbytes_read < 0) {
        print_failure_message("File reader child: can't read from the input file");
        return false;
    }

    if (write(fd_reader_to_processor[1], read_buffer, (size_t)nbytes_read) < 0) {
        print_failure_message(
            "File reader child: can't send data to another process using pipe [file "
            "reader] -> [text processor]");
        return false;
    }

    if (close(fd_reader_to_processor[1]) < 0) {
        print_failure_message(
            "File reader child: can't close writing side of the pipe [file reader] -> "
            "[text processor]");
        return false;
    }
    debug_print("reader closed [reader] -> [processor]");
    fd_reader_to_processor[1] = -1;

    nbytes_read = read(fd_processor_to_writer[0], read_buffer, BUFFER_SIZE - 1);
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

    if (close(fd_processor_to_writer[0]) < 0) {
        print_failure_message(
            "File writer child: can't close reading side of the pipe [text processor] -> "
            "[file writer]");
        return false;
    }
    debug_print("writer closed [processor] -> [writer]");
    fd_processor_to_writer[0] = -1;

    return true;
}

static pid_t create_file_reader_writer(const char* input_filename,
                                       int fd_reader_to_processor[2],
                                       const char* output_filename,
                                       int fd_processor_to_writer[2]) {
    pid_t child_pid = fork();
    if (child_pid != 0) {
        return child_pid;
    }

    int read_file_fd  = open(input_filename, O_RDONLY);
    int write_file_fd = open(output_filename, O_WRONLY | O_CREAT, 0644);
    bool result = create_file_reader_writer_impl(read_file_fd, fd_reader_to_processor,
                                                 write_file_fd, fd_processor_to_writer);
    if (read_file_fd >= 0) {
        if (close(read_file_fd) < 0) {
            print_failure_message("File reader child: can't close opened input file");
            result = false;
        }
    }
    if (fd_reader_to_processor[0] != -1) {
        if (close(fd_reader_to_processor[0]) >= 0) {
            debug_print("reader closed [reader] <- [processor]");
        }
    }
    if (fd_reader_to_processor[1] != -1) {
        if (close(fd_reader_to_processor[1]) >= 0) {
            debug_print("reader closed [reader] -> [processor]");
        }
    }
    if (write_file_fd >= 0) {
        if (close(write_file_fd) < 0) {
            print_failure_message("File writer child: can't close opened output file");
            result = false;
        }
    }
    if (fd_processor_to_writer[0] != -1) {
        if (close(fd_processor_to_writer[0]) >= 0) {
            debug_print("writer closed [processor] -> [writer]");
        }
    }
    if (fd_processor_to_writer[1] != -1) {
        if (close(fd_processor_to_writer[1]) >= 0) {
            debug_print("writer closed [processor] <- [writer]");
        }
    }

    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool create_data_processor_impl(uint32_t start_n, uint32_t end_n,
                                       int fd_reader_to_processor[2],
                                       int fd_processor_to_writer[2]) {
    if (close(fd_reader_to_processor[1]) < 0) {
        print_failure_message(
            "Data processor child: can't close writing side of the pipe [file reader] -> "
            "[text processor]");
        return false;
    }
    debug_print("processor closed [reader] <- [processor]");
    fd_reader_to_processor[1] = -1;

    if (close(fd_processor_to_writer[0]) < 0) {
        print_failure_message(
            "Data processor child: can't close reading side of the pipe [text processor] "
            "-> [file writer]");
        return false;
    }
    debug_print("processor closed [processor] <- [writer]");
    fd_processor_to_writer[0] = -1;

    char read_buffer[BUFFER_SIZE];
    ssize_t nbytes_read = read(fd_reader_to_processor[0], read_buffer, BUFFER_SIZE - 1);
    if (nbytes_read < 0) {
        print_failure_message(
            "Data processor child: can't read from pipe [file reader] -> [text "
            "processor]");
        return false;
    }

    if (nbytes_read == 0) {
        print_failure_message(
            "Data processor child: empty input from pipe [file reader] -> [text "
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
    if (write(fd_processor_to_writer[1], read_buffer, string_length) < 0) {
        print_failure_message(
            "Data processor child: can't send data to another process using pipe [text "
            "processor] -> [file writer]");
        return false;
    }

    if (close(fd_reader_to_processor[0]) < 0) {
        print_failure_message(
            "Data processor child: can't close reading side of the pipe [file reader] -> "
            "[text processor]");
        return false;
    }
    debug_print("processor closed [reader] -> [processor]");
    fd_reader_to_processor[0] = -1;
    if (close(fd_processor_to_writer[1]) < 0) {
        print_failure_message(
            "Data processor child: can't close writing side of the pipe [text processor] "
            "-> [file writer]");
        return false;
    }
    debug_print("processor closed [processor] -> [writer]");
    fd_processor_to_writer[1] = -1;
    return true;
}

static pid_t create_data_processor(uint32_t start_n, uint32_t end_n,
                                   int fd_reader_to_processor[2],
                                   int fd_processor_to_writer[2]) {
    assert(start_n <= end_n);
    pid_t child_pid = fork();
    if (child_pid != 0) {
        return child_pid;
    }

    bool result = create_data_processor_impl(start_n, end_n, fd_reader_to_processor,
                                             fd_processor_to_writer);

    if (fd_reader_to_processor[0] != -1) {
        if (close(fd_reader_to_processor[0]) >= 0) {
            debug_print("processor closed [reader] -> [processor]");
        }
    }
    if (fd_reader_to_processor[1] != -1) {
        if (close(fd_reader_to_processor[1]) >= 0) {
            debug_print("processor closed [reader] <- [processor]");
        }
    }
    if (fd_processor_to_writer[0] != -1) {
        if (close(fd_processor_to_writer[0]) >= 0) {
            debug_print("processor closed [processor] <- [writer]");
        }
    }
    if (fd_processor_to_writer[1] != -1) {
        if (close(fd_processor_to_writer[1]) >= 0) {
            debug_print("processor closed [processor] -> [writer]");
        }
    }

    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool run(parse_result_t parse_result) {
    int fd_reader_to_processor[2] = {0};
    int fd_processor_to_writer[2] = {0};

    if (pipe(fd_reader_to_processor) < 0) {
        print_failure_message(
            "Main process: Can't create pipe [file reader] -> [text processor]");
        return false;
    }

    if (pipe(fd_processor_to_writer)) {
        print_failure_message(
            "Main process: Can't create pipe [text processor] -> [file writer]");
        return false;
    }

    pid_t file_reader_pid =
        create_file_reader_writer(parse_result.input_filename, fd_reader_to_processor,
                                  parse_result.output_filename, fd_processor_to_writer);
    if (file_reader_pid == -1) {
        print_failure_message(
            "Main process: Can't create child process for reading and file");
        return false;
    }

    pid_t data_processor_pid =
        create_data_processor(parse_result.start_n1, parse_result.end_n2,
                              fd_reader_to_processor, fd_processor_to_writer);
    if (data_processor_pid == -1) {
        print_failure_message(
            "Main process: Can't create child process for processing data");
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
