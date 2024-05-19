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

static void run_third_worker(SharedMemory* second_to_third_sh_mem) {
    assert(second_to_third_sh_mem);
    while (true) {
        Pin sharpened_pin;
        if (!wait_for_pin(second_to_third_sh_mem, &sharpened_pin)) {
            break;
        }

        printf(
            "+--------------------------------------------------------------\n"
            "| Third worker received sharpened\n"
            "| pin[pin_id=%d] and started checking it's quality...\n"
            "+--------------------------------------------------------------\n",
            sharpened_pin.pin_id);
        bool is_ok = check_sharpened_pin_quality(sharpened_pin);
        printf(
            "+--------------------------------------------------------------\n"
            "| Third worker decision:\n"
            "| pin[pin_id=%d] is sharpened %s.\n"
            "+--------------------------------------------------------------\n",
            sharpened_pin.pin_id, (is_ok ? "good enough" : "badly"));
    }

    fputs("Third worker finished with error\n", stderr);
    exit(EXIT_FAILURE);
}

int main(void) {
    setup_signal_handlers(&shutdown_signal_handler);

    InitStatus res =
        init_shared_memory_info(&sh_mem_info, SECOND_TO_THIRD_WORKER);
    if (res == SHARED_MEMORY_INIT_FAILURE) {
        return EXIT_FAILURE;
    }
    run_third_worker(sh_mem_info.sh_mem);
    deinit_shared_memory_info(&sh_mem_info);
    return EXIT_SUCCESS;
}
