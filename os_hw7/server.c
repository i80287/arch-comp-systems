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

static bool is_running = true;
static pid_t client_id = -1;

static void interrupt_signal_handler(int signal) {
    is_running = false;
}

static void exchange_process_ids_with_client(message_t* message_ptr) {
    const pid_t server_pid = getpid();
    message_ptr->rand_num  = server_pid;
    message_ptr->type      = SERVER_SENT_PROCESS_ID;
    printf("[server] [>>>] sent id %d of this process to client\n",
           server_pid);

    while (is_running && message_ptr->type != CLIENT_SENT_PROCESS_ID) {
        sleep_nanos(DEFAULT_SLEEP_SPINLOCK_INTERVAL);
    }
    client_id = (pid_t)message_ptr->rand_num;

    printf("[server] [>>>] got process id %d of the client\n", client_id);
    message_ptr->type = SERVER_GOT_PROCESS_ID;
}

static void setup_signal_handlers() {
    signal(SIGINT, interrupt_signal_handler);
    if (TERMINATE_USING_SIGNAL) {
        signal(SIGUSR1, interrupt_signal_handler);
    }
}

static void run_server(message_t* message_ptr) {
    message_ptr->type = EMPTY_MESSAGE;
    printf(
        "[server] [>>>] got access to the shared memory block at address %p\n",
        (void*)message_ptr);

    if (TERMINATE_USING_SIGNAL) {
        exchange_process_ids_with_client(message_ptr);
    }

    bool client_shutdowned_first = false;
    while (is_running) {
        switch (message_ptr->type) {
            case NEW_NUMBER:
                printf(
                    "[server] [>>>] got new number from the client: %" PRId64 "\n",
                    message_ptr->rand_num);
                message_ptr->type = EMPTY_MESSAGE;
                __attribute__((fallthrough));
            case EMPTY_MESSAGE:
                sleep_nanos(DEFAULT_SLEEP_SPINLOCK_INTERVAL);
                break;
            case CLIENT_SHUTDOWN:
                client_shutdowned_first = true;
                is_running              = false;
                break;
            default:
                break;
        }
    }

    if (!client_shutdowned_first) {
        if (TERMINATE_USING_SIGNAL) {
            kill(client_id, SIGUSR1);
        } else {
            message_ptr->type          = SERVER_SHUTDOWN;
            uint32_t retries           = 0;
            const uint32_t MAX_RETRIES = 64;
            while (message_ptr->type != READY_TO_REMOVE_SEGMENT &&
                   ++retries <= MAX_RETRIES) {
                sleep_nanos(DEFAULT_SLEEP_SPINLOCK_INTERVAL);
            }
        }
    }
}

int main(void) {
    setup_signal_handlers();

    int shared_mem_id =
        shm_open(shared_object_name, SHARED_MEMORY_OPEN_FLAGS,
                 SHARED_MEMORY_OPEN_PERMS_MODE);
    if (shared_mem_id == -1) {
        perror("[server] [>>>] error: shm_open");
        return EXIT_FAILURE;
    }
    if (ftruncate(shared_mem_id, SHARED_MEMORY_SIZE) == -1) {
        perror("[server] [>>>] error: ftruncate");
        shm_unlink(shared_object_name);
        return EXIT_FAILURE;
    }

    printf(
        "[server] [>>>] got access to the shared memory with key %d and size %d bytes\n",
        shared_mem_id, SHARED_MEMORY_SIZE);

    message_t* message_ptr =
        (message_t*)mmap(0, SHARED_MEMORY_SIZE, MMAP_PROT,
                         MMAP_FLAGS, shared_mem_id, 0);
    if (message_ptr == (message_t*)-1) {
        perror("[server] [>>>] error: mmap");
        shm_unlink(shared_object_name);
        return EXIT_FAILURE;
    }

    run_server(message_ptr);

    close(shared_mem_id);
    if (shm_unlink(shared_object_name) == -1) {
        perror("[server] [>>>] error: shmctl");
        return EXIT_FAILURE;
    }
    printf("[server] [>>>] deleted shared memory with id %d\n",
           shared_mem_id);

    return EXIT_SUCCESS;
}
