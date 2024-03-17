#include <assert.h>
#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "helper_functions.h"

typedef struct {
    uint32_t read_number;
    uint32_t write_bit;
    pid_t other_pid;
    bool other_process_ready;
} program_state_t;

static program_state_t program_state = {
    .read_number         = 0,
    .write_bit           = 0,
    .other_pid           = -1,
    .other_process_ready = false,
};

static void signal_handler(int signal) {
    if (signal != ZERO_BIT_SIGNAL && signal != ONE_BIT_SIGNAL) {
        return;
    }
    if (signal == READY_SIGNAL && !program_state.other_process_ready) {
        program_state.other_process_ready = true;
        return;
    }

    uint32_t bit = signal == ONE_BIT_SIGNAL;
    program_state.read_number |= bit << program_state.write_bit;
    program_state.write_bit++;

    printf("%u", bit);
    fflush(stdout);

    assert(check_process_id(program_state.other_pid));
    kill(program_state.other_pid, RECEIVED_APPROVE_SIGNAL);
}

static void setup_signal_handlers() {
    signal(ZERO_BIT_SIGNAL, signal_handler);
    signal(ONE_BIT_SIGNAL, signal_handler);
}

int main() {
    setup_signal_handlers();

    printf("Process id of this program: %d\n", getpid());
    do {
        program_state.other_pid =
            (pid_t)read_uint32("Input process id of the other program:\n> ");
    } while (!check_process_id(program_state.other_pid));

    send_ready_signal(program_state.other_pid);
    while (!program_state.other_process_ready) {
        pause();
    }

    while (program_state.write_bit < sizeof(int32_t) * CHAR_BIT) {
        pause();
    }

    printf("\nResult: %d\n", (int32_t)program_state.read_number);
    fflush(stdout);
}
