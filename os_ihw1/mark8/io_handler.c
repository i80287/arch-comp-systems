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
#include <unistd.h>

static const size_t BUFFER_SIZE         = 5000 * sizeof(char);
static const char READER_TO_PROC_FIFO[] = "reader-to-proc.fifo";
static const char PROC_TO_WRITER_FIFO[] = "proc-to-writer.fifo";

static void print_failure_message(const char* failure_message) {
    fprintf(stderr, "[>>>] Error: %s\n", failure_message);
}

static bool run_impl(int read_file_fd, int reader_to_proc_fd, int write_file_fd,
                     int processor_to_writer_fd) {
    if (read_file_fd < 0) {
        print_failure_message("File reader child: can't open input file");
        return false;
    }
    if (reader_to_proc_fd < 0) {
        print_failure_message(
            "File reader child: can't open fifo [file reader] -> [text processor]");
        return false;
    }
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

    nbytes_read = read(processor_to_writer_fd, read_buffer, BUFFER_SIZE - 1);
    if (nbytes_read < 0) {
        print_failure_message(
            "File writer child: can't read from the fifo [text processor] -> [file "
            "writer]");
        return false;
    }

    if (write(write_file_fd, read_buffer, (size_t)nbytes_read) < 0) {
        print_failure_message("File writer child: can't write to output file");
        return false;
    }

    return true;
}

static bool run(const char* input_filename, const char* output_filename) {
    umask(0);
    mknod(READER_TO_PROC_FIFO, S_IFIFO | 0666, 0);
    umask(0);
    mknod(PROC_TO_WRITER_FIFO, S_IFIFO | 0666, 0);

    int read_file_fd           = open(input_filename, O_RDONLY);
    int reader_to_proc_fd      = open(READER_TO_PROC_FIFO, O_WRONLY);
    int write_file_fd          = open(output_filename, O_WRONLY | O_CREAT, 0644);
    int processor_to_writer_fd = open(PROC_TO_WRITER_FIFO, O_RDONLY);
    bool result =
        run_impl(read_file_fd, reader_to_proc_fd, write_file_fd, processor_to_writer_fd);
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
    if (write_file_fd >= 0) {
        if (close(write_file_fd) < 0) {
            print_failure_message("File writer child: can't close opened output file");
            result = false;
        }
    }
    if (processor_to_writer_fd >= 0) {
        if (close(processor_to_writer_fd) < 0) {
            print_failure_message(
                "File writer child: can't close opened fifo [text processor] -> [file "
                "writer]");
            result = false;
        }
    }

    return result;
}

int main(int argc, const char* const argv[]) {
    if (argc != 3) {
        fputs(
            "[>>>] Error        : expeted 2 arguments\n"
            "[>>>] Usage hint   : executable input-file-name output-file-name\n"
            "[>>>] Usage example: main.out in.txt out.txt\n",
            stderr);
        return EXIT_FAILURE;
    }

    bool result = run(argv[1], argv[2]);
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
