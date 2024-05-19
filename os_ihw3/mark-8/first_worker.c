#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "workers_functions.h"

static SharedMemoryInfo sh_mem_info;

static void shutdown_signal_handler(int signal) {
    int ret_status = EXIT_FAILURE;
    switch (signal) {
        default:
            return;
        case SIGINT:
            ret_status = EXIT_SUCCESS;
            __attribute__((fallthrough));
        case SIGKILL:
        case SIGABRT:
        case SIGTERM:
            deinit_shared_memory_info(&sh_mem_info);
            break;
    }

    puts("Worker stopped working");
    exit(ret_status);
}

static void run_first_worker(SharedMemory* first_to_second_sh_mem) {
    assert(first_to_second_sh_mem);
    while (true) {
        const Pin produced_pin = {.pin_id = rand()};
        printf(
            "+--------------------------------------------------------------\n"
            "| First worker received pin[pin_id=%d]\n"
            "| and started checking it's crookness...\n"
            "+--------------------------------------------------------------\n",
            produced_pin.pin_id);

        bool is_ok = check_pin_crookness(produced_pin);
        printf(
            "+--------------------------------------------------------------\n"
            "| First worker decision:\n"
            "| pin[pin_id=%d] is %scrooked.\n"
            "+--------------------------------------------------------------\n",
            produced_pin.pin_id, (is_ok ? "not " : ""));
        if (!is_ok) {
            continue;
        }

        if (!send_produced_pin(first_to_second_sh_mem, produced_pin)) {
            break;
        }
    }

    fputs("First worker finished with error\n", stderr);
    exit(EXIT_FAILURE);
}

int main(void) {
    setup_signal_handlers(&shutdown_signal_handler);

    InitStatus res =
        init_shared_memory_info(&sh_mem_info, FIRST_TO_SECOND_WORKER);
    if (res == SHARED_MEMORY_INIT_FAILURE) {
        return EXIT_FAILURE;
    }
    run_first_worker(sh_mem_info.sh_mem);
    deinit_shared_memory_info(&sh_mem_info);
    return EXIT_SUCCESS;
}
