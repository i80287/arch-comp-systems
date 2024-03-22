// Defined to define _POSIX_C_SOURCE 200809L to enable nanosleep
#define _XOPEN_SOURCE 700

#include <inttypes.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "message.h"
#include "sleep_nanos.h"

static bool is_running = true;

static void interrupt_signal_handler(int signal) {
    is_running = false;
}

int main(void) {
    signal(SIGINT, interrupt_signal_handler);

    int shared_mem_id =
        shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, SHARED_MEMORY_PERMS | IPC_CREAT);
    if (shared_mem_id == -1) {
        perror("[server] [>>>] error: shmget");
        return EXIT_FAILURE;
    }

    printf("[server] [>>>] got access to the shared memory with key %d\n", shared_mem_id);

    message_t* message_ptr = (message_t*)shmat(shared_mem_id, 0, 0);
    if (message_ptr == NULL) {
        perror("[server] [>>>] error: shmat");
        return EXIT_FAILURE;
    }

    printf("[server] [>>>] got access to the shared memory block at address %p\n",
           (void*)message_ptr);
    message_ptr->type = EMPTY_MESSAGE;

    bool client_shutdowned_first = false;
    while (is_running) {
        switch (message_ptr->type) {
            case NEW_NUMBER:
                printf("[server] [>>>] got new number from the client: %" PRIu64 "\n",
                       message_ptr->rand_num);
                message_ptr->type = EMPTY_MESSAGE;
                __attribute__((fallthrough));
            case EMPTY_MESSAGE:
                sleep_nanos(20000);
                break;
            case CLIENT_SHUTDOWN_SIGNAL:
                client_shutdowned_first = true;
                is_running              = false;
                break;
            default:
                printf("SERVER ERROR: unknown code %d\n", message_ptr->type);
                break;
        }
    }

    if (!client_shutdowned_first) {
        message_ptr->type = SERVER_SHUTDOWN_SIGNAL;
        while (message_ptr->type != READY_TO_REMOVE_SEGMENT) {
            sleep_nanos(20000);
        }
    }

    shmdt(message_ptr);
    if (shmctl(shared_mem_id, IPC_RMID, NULL) == -1) {
        perror("[server] [>>>] error: shmctl");
        return EXIT_FAILURE;
    }
    printf("[server] [>>>] deleted shared memory with id %d\n", shared_mem_id);
    return EXIT_SUCCESS;
}
