/**
 * This work is done in accordance
 * with the criteria for 10 points.
 */

// Defined for the strsignal in string.h
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/// @brief Handler for the signals sent by the OS.
/// @param[in] signal_value
static void signals_handler(int signal_value) {
    int errno_value = errno;
    fprintf(stderr,
            "[>>>] Error occured:\n"
            "[>>>] Signal %d: %s\n"
            "[>>>] Errno %d: %s\n",
            signal_value, strsignal(signal_value), errno_value, strerror(errno_value));
    errno = 0;
}

/// @brief Function that registers appropriate handlers for the signals.
static void setup_signal_handlers() {
    signal(SIGABRT, &signals_handler);
    signal(SIGFPE, &signals_handler);
    signal(SIGILL, &signals_handler);
    signal(SIGINT, &signals_handler);
    signal(SIGSEGV, &signals_handler);
    signal(SIGTERM, &signals_handler);
}

/// @brief Function that prints info about current IO error
///         regarding @a `filepath` stored in the errno.
///        May be called by the @ref `copy_file_impl` if IO error occurs.
/// @param[in] filepath
static void handle_file_error(const char* filepath) {
    int errno_value = errno;
    fprintf(stderr,
            "[>>>] IO error occured\n"
            "[>>>] File: %s\n"
            "[>>>] Errno %d: %s\n",
            filepath ? filepath : "", errno_value, strerror(errno_value));
    errno = 0;
}

/// @brief Function that prints that specified @a `filepath` is not a
///         regular file and hence can't be copied.
///        May be called by the @ref `copy_file_impl` if IO error occurs.
/// @param[in] filepath
static void handle_not_regular_file(const char* filepath) {
    fprintf(stderr,
            "[>>>] IO error occured\n"
            "[>>>] File %s is not a regular file and can't be copied\n",
            filepath ? filepath : "");
}

/// @brief Pair of file descriptors.
///        Used to be returned from copy_file_impl and closed by copy_file.
struct descriptors_pair {
    int dst_fd;
    int src_fd;
};

/// @brief Implementation of copy_file_impl
/// @param[in] src_filepath path to the source file (will be copied to the @a
/// `dst_filepath` )
/// @param[in] dst_filepath path to the destination file (may not exist)
/// @return pair of file descriptors each to be closed if greater then @ref
/// `STDERR_FILENO`
static struct descriptors_pair copy_file_impl(const char* src_filepath,
                                              const char* dst_filepath) {
    struct descriptors_pair dp = {-1, -1};
    dp.src_fd                  = open(src_filepath, O_RDONLY);
    if (dp.src_fd < 0) {
        handle_file_error(src_filepath);
        return dp;
    }

    struct stat src_file_statistic;
    // This functions makes appropriate syscalls to fetch file's information
    if (fstat(dp.src_fd, &src_file_statistic) < 0) {
        handle_file_error(src_filepath);
        return dp;
    }

    if (!S_ISREG(src_file_statistic.st_mode)) {
        handle_not_regular_file(src_filepath);
        return dp;
    }

    const mode_t MODE_MASK = 0777;
    mode_t dst_file_mode   = src_file_statistic.st_mode & MODE_MASK;
    dp.dst_fd              = open(dst_filepath, O_WRONLY | O_CREAT, dst_file_mode);
    if (dp.dst_fd < 0) {
        handle_file_error(dst_filepath);
        return dp;
    }

    // It's better to use sendfile here.

    char copy_buffer[8192 + 1]   = {0};
    const uint32_t MAX_READ_SIZE = sizeof(copy_buffer) / sizeof(char) - 1;
    ssize_t real_read_size       = 0;
    do {
        real_read_size = read(dp.src_fd, copy_buffer, MAX_READ_SIZE);
        if (real_read_size < 0) {
            handle_file_error(src_filepath);
            return dp;
        }

        if (write(dp.dst_fd, copy_buffer, (size_t)real_read_size) < 0) {
            handle_file_error(dst_filepath);
            return dp;
        }
    } while (real_read_size == MAX_READ_SIZE);

    return dp;
}

/// @brief Copies content of the file specified by the @a `src_filepath`
///         to the @a `dst_filepath`. If @a `dst_filepath` does not exists,
///         it is created with the same mode as source file.
///        If IO error occured, appropriate handler is called and error
///         message is prited to the @ref `stderr`.
/// @param src_filepath path to the source file (will be copied to the @a `dst_filepath` )
/// @param dst_filepath path to the destination file (may not exist)
void copy_file(const char* src_filepath, const char* dst_filepath) {
    struct descriptors_pair fp = copy_file_impl(src_filepath, dst_filepath);
    if (fp.src_fd > STDERR_FILENO) {
        close(fp.src_fd);
    }
    if (fp.dst_fd > STDERR_FILENO) {
        close(fp.dst_fd);
    }
}

int main(int argc, const char* const argv[]) {
    setup_signal_handlers();

    // argv[0] is an executable path.
    if (argc != 3) {
        fprintf(
            stderr,
            "[>>>] Incorrect input arguments: 2 arguments representing file names and/or "
            "paths were expected\n"
            "[>>>] Usage: %s source_path destination_path\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    copy_file(argv[1], argv[2]);
}
