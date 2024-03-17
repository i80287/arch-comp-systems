#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "helper_functions.h"

typedef struct {
    sem_t sem;
    bool other_process_ready;
} program_state_t;

static program_state_t program_state = {
    .other_process_ready = false,
};

static void approve_signal_handler(int signal) {
    if (signal == RECEIVED_APPROVE_SIGNAL) {
        sem_post(&program_state.sem);
    }
}

static void ready_signal_handler(int signal) {
    if (signal == READY_SIGNAL) {
        program_state.other_process_ready = true;
    }
}

static void init_program_state() {
    sem_init(&program_state.sem, false, 1);
}

static void deinit_program_state() { sem_destroy(&program_state.sem); }

static void send_last_bit(pid_t other_pid, uint32_t number) {
    const int bit_to_signal[] = {
        [0] = ZERO_BIT_SIGNAL,
        [1] = ONE_BIT_SIGNAL,
    };
    kill(other_pid, bit_to_signal[number & 1]);
}

static void wait_for_approval() {
    sem_wait(&program_state.sem);
    sleep(1);
}

static void setup_signal_handlers() {
    signal(RECEIVED_APPROVE_SIGNAL, approve_signal_handler);
    signal(READY_SIGNAL, ready_signal_handler);
}

int main() {
    setup_signal_handlers();
    init_program_state();

    printf("Process id of this program: %d\n", getpid());
    pid_t other_pid;
    do {
        other_pid = (pid_t)read_uint32("Input process id of the other program:\n> ");
    } while (!check_process_id(other_pid));

    send_ready_signal(other_pid);
    while (!program_state.other_process_ready) {
        pause();
    }

    uint32_t number_to_send = (uint32_t)read_int32(
        "Input decimal integer number between -2147483648 and 2147483647:\n> ");
    printf("Sending %d to the %d\n", (int32_t)number_to_send, other_pid);
    for (uint32_t bits = sizeof(int32_t) * CHAR_BIT; bits != 0;
         bits--, number_to_send >>= 1) {
        send_last_bit(other_pid, number_to_send);
        wait_for_approval();
    }

    deinit_program_state();
}
