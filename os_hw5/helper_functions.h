#ifndef HEPLER_FUNCTIONS_H
#define HEPLER_FUNCTIONS_H

// For the getpgid() support
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

/// @brief Constants used in the program.
enum {
    RECEIVED_APPROVE_SIGNAL = SIGUSR1,
    READY_SIGNAL = SIGUSR2,
    ZERO_BIT_SIGNAL = SIGUSR1,
    ONE_BIT_SIGNAL = SIGUSR2,
};

static uint32_t read_uint32(const char* prompt) {
    uint32_t num = 0;
    fputs(prompt, stdout);
    fflush(stdout);
    while (1) {
        if (scanf("%u", &num) == 1) {
            return num;
        }

        fputs(
            "Expected non-negative integer number less than 4294967296. Please, try "
            "again\n> ",
            stdout);
        fflush(stdout);
    }
}

static int32_t read_int32(const char* prompt) {
    int32_t num = 0;
    fputs(prompt, stdout);
    fflush(stdout);
    while (1) {
        if (scanf("%d", &num) == 1) {
            return num;
        }

        fputs(
            "Expected integer number between -2147483648 and 2147483647. Please, try "
            "again\n> ",
            stdout);
        fflush(stdout);
    }
}

static bool check_process_id(pid_t pid) {
    return pid >= 0 && getpgid(pid) >= 0 && pid != getpid();
}

static void send_ready_signal(pid_t other_pid) {
    kill(other_pid, READY_SIGNAL);
}

#endif
