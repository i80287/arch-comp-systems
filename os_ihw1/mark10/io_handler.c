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
#include <unistd.h>

static const size_t TRANSFER_BUFFER_SIZE      = 128 * sizeof(char);
static const char END_OF_COMMUNICATION_CHAR   = (char)128;
static const char READER_TO_PROC_QUEUE_NAME[] = "/reader-to-proc-q";
static const char PROC_TO_WRITER_QUEUE_NAME[] = "/proc-to-writer-q";

static void print_failure_message(const char* failure_message) {
    perror(failure_message);
    fprintf(stderr, "[>>>] Error: %s\n", failure_message);
}

static bool run_impl(int read_file_fd, mqd_t reader_to_proc_q, int write_file_fd,
                     mqd_t processor_to_writer_q) {
    if (read_file_fd == -1) {
        print_failure_message("File reader: can't open input file");
        return false;
    }
    if (reader_to_proc_q == -1) {
        print_failure_message(
            "File reader: can't open queue [file reader] -> [text processor]");
        return false;
    }
    if (write_file_fd == -1) {
        print_failure_message("File writer: can't open output file");
        return false;
    }
    if (processor_to_writer_q == -1) {
        print_failure_message(
            "File writer: can't open queue [text processor] -> [file writer]");
        return false;
    }

    while (true) {
        char read_buffer[TRANSFER_BUFFER_SIZE];
        ssize_t nbytes_read = read(read_file_fd, read_buffer, TRANSFER_BUFFER_SIZE - 1);
        if (nbytes_read < 0) {
            print_failure_message("File reader: can't read from the input file");
            return false;
        }

        if (nbytes_read == 0) {
            break;
        }

        if (mq_send(reader_to_proc_q, read_buffer, (size_t)nbytes_read, 0) == -1) {
            print_failure_message(
                "File reader: can't send data to another process using queue [file "
                "reader] -> [text processor]");
            return false;
        }

        nbytes_read =
            mq_receive(processor_to_writer_q, read_buffer, TRANSFER_BUFFER_SIZE - 1, 0);
        if (nbytes_read < 0) {
            print_failure_message(
                "File writer: can't read from the queue [text processor] -> [file "
                "writer]");
            return false;
        }

        if (write(write_file_fd, read_buffer, (size_t)nbytes_read) < 0) {
            print_failure_message("File writer: can't write to output file");
            return false;
        }
    }

    return true;
}

static void notify_end_of_communication(mqd_t reader_to_proc_q) {
    if (reader_to_proc_q >= 0) {
        const char end_message[] = {END_OF_COMMUNICATION_CHAR};
        mq_send(reader_to_proc_q, end_message, sizeof(end_message), 0);
    }
}

static bool run(const char* input_filename, const char* output_filename) {
    int read_file_fd = open(input_filename, O_RDONLY);
    int write_file_fd = open(output_filename, O_WRONLY | O_CREAT, 0644);
    struct mq_attr mqattrs = { 0 };
    mqattrs.mq_maxmsg = 10;
    mqattrs.mq_msgsize = (long)(TRANSFER_BUFFER_SIZE - 1);
    mqd_t reader_to_proc_q = mq_open(READER_TO_PROC_QUEUE_NAME, O_WRONLY | O_CREAT, 0666, &mqattrs);
    mqd_t processor_to_writer_q = mq_open(PROC_TO_WRITER_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &mqattrs);
    bool result = run_impl(read_file_fd, reader_to_proc_q, write_file_fd, processor_to_writer_q);
    // Send message about end of communication between processes
    notify_end_of_communication(reader_to_proc_q);

    if (read_file_fd >= 0) {
        if (close(read_file_fd) < 0) {
            print_failure_message("File reader: can't close opened input file");
            result = false;
        }
    }
    if (reader_to_proc_q != -1) {
        if (mq_close(reader_to_proc_q) == -1) {
            print_failure_message(
                "File reader: can't close queue [file reader] -> [text processor]");
            result = false;
        }
        mq_unlink(READER_TO_PROC_QUEUE_NAME);
    }
    if (write_file_fd >= 0) {
        if (close(write_file_fd) < 0) {
            print_failure_message("File writer: can't close opened output file");
            result = false;
        }
        
    }
    if (processor_to_writer_q != -1) {
        if (mq_close(processor_to_writer_q) == -1) {
            print_failure_message(
                "File reader: can't close queue text processor] -> [file writer]");
            result = false;
        }
        mq_unlink(PROC_TO_WRITER_QUEUE_NAME);
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
