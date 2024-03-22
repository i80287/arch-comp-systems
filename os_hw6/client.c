// Defined to define _POSIX_C_SOURCE 200809L to enable kill() function
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

#include "message.h"
#include "sleep_nanos.h"

static bool is_running  = true;
static pid_t server_pid = -1;

static void interrupt_signal_handler(int signal) { is_running = false; }

static void exchange_process_ids_with_server(message_t* message_ptr) {
    while (is_running && message_ptr->type != SERVER_SENT_PROCESS_ID) {
        sleep_nanos(DEFAULT_SLEEP_SPINLOCK_INTERVAL);
    }
    server_pid = (pid_t)message_ptr->rand_num;
    printf("[client] [>>>] got server process id %d\n", server_pid);

    const pid_t client_id = getpid();
    message_ptr->rand_num = client_id;
    message_ptr->type     = CLIENT_SENT_PROCESS_ID;
    while (is_running && message_ptr->type != SERVER_GOT_PROCESS_ID) {
        sleep_nanos(DEFAULT_SLEEP_SPINLOCK_INTERVAL);
    }
    printf("[client] [>>>] server recieved id %d of this process\n", client_id);
}

static void setup_signal_handlers() {
    signal(SIGINT, interrupt_signal_handler);
    if (TERMINATE_USING_SIGNAL) {
        signal(SIGUSR1, interrupt_signal_handler);
    }
}

static void run_client(message_t* message_ptr) {
    if (TERMINATE_USING_SIGNAL) {
        exchange_process_ids_with_server(message_ptr);
    }

    const time_t NUMBER_GENERATION_DELAY_SECONDS = 2;
    time_t last_number_send_time                 = 0;
    bool server_shutdowned_first                 = false;
    while (is_running) {
        server_shutdowned_first = message_ptr->type == SERVER_SHUTDOWN;
        if (server_shutdowned_first) {
            break;
        }

        time_t time_now_unix_epoch = time(NULL);
        if (time_now_unix_epoch - last_number_send_time >=
            NUMBER_GENERATION_DELAY_SECONDS) {
            int64_t rand_num      = random();
            message_ptr->rand_num = rand_num;
            message_ptr->type     = NEW_NUMBER;
            printf("[client] [>>>] Generated and sent number %" PRId64 "\n", rand_num);
            last_number_send_time = time_now_unix_epoch;
        }

        sleep_nanos(DEFAULT_SLEEP_SPINLOCK_INTERVAL);
    }

    if (!server_shutdowned_first) {
        if (TERMINATE_USING_SIGNAL) {
            kill(server_pid, SIGUSR1);
        } else {
            message_ptr->type = CLIENT_SHUTDOWN;
        }
    } else {
        message_ptr->type = READY_TO_REMOVE_SEGMENT;
    }
}

int main(void) {
    setup_signal_handlers();

    int shared_mem_id =
        shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, SHARED_MEMORY_PERMS | IPC_CREAT);
    if (shared_mem_id == -1) {
        perror("[client] [>>>] error: shmget");
        return EXIT_FAILURE;
    }

    printf("[client] [>>>] got access to the shared memory with key %d\n", shared_mem_id);

    message_t* message_ptr = (message_t*)shmat(shared_mem_id, 0, 0);
    if (message_ptr == NULL) {
        perror("[client] [>>>] error: shmat");
        return EXIT_FAILURE;
    }

    printf("[client] [>>>] got access to the shared memory block at address %p\n",
           (void*)message_ptr);

    run_client(message_ptr);

    shmdt(message_ptr);
    printf("[client] [>>>] stopped generating numbers\n");
    return EXIT_SUCCESS;
}
