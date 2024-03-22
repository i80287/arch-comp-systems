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

static void interrupt_signal_handler(int signal) { is_running = false; }

int main(void) {
    signal(SIGINT, interrupt_signal_handler);

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

    const time_t NUMBER_GENERATION_DELAY_SECONDS = 2;
    time_t last_number_send_time                 = 0;
    bool server_shutdowned_first                 = false;
    while (is_running) {
        server_shutdowned_first = message_ptr->type == SERVER_SHUTDOWN_SIGNAL;
        if (server_shutdowned_first) {
            break;
        }

        time_t time_now_unix_epoch = time(NULL);
        if (time_now_unix_epoch - last_number_send_time >=
            NUMBER_GENERATION_DELAY_SECONDS) {
            uint64_t rand_num     = (uint64_t)random();
            message_ptr->rand_num = rand_num;
            message_ptr->type     = NEW_NUMBER;
            printf("[client] [>>>] Generated and sent number %" PRIu64 "\n", rand_num);
            last_number_send_time = time_now_unix_epoch;
        }

        sleep_nanos(20000);
    }

    message_ptr->type =
        server_shutdowned_first ? READY_TO_REMOVE_SEGMENT : CLIENT_SHUTDOWN_SIGNAL;
    shmdt(message_ptr);

    printf("[client] [>>>] stopped generating numbers\n");

    return EXIT_SUCCESS;
}
