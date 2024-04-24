#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "workers_functions.h"

static SharedMemoryInfo sh_mem_info_first_to_second;
static SharedMemoryInfo sh_mem_info_second_to_third;

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
            deinit_shared_memory_info(&sh_mem_info_second_to_third);
            deinit_shared_memory_info(&sh_mem_info_first_to_second);
            break;
    }

    puts("Worker stopped working");
    exit(ret_status);
}

static void run_second_worker(SharedMemory* first_to_second_sh_mem,
                              SharedMemory* second_to_third_sh_mem) {
    assert(first_to_second_sh_mem);
    assert(second_to_third_sh_mem);
    while (true) {
        Pin pin;
        if (!wait_for_pin(first_to_second_sh_mem, &pin)) {
            break;
        }

        printf(
            "+--------------------------------------------------------------\n"
            "| Second worker received pin[pin_id=%d]\n"
            "| and started sharpening it...\n"
            "+--------------------------------------------------------------\n",
            pin.pin_id);
        sharpen_pin(pin);
        printf(
            "+--------------------------------------------------------------\n"
            "| Second worker sharpened pin[pin_id=%d].\n"
            "+--------------------------------------------------------------\n",
            pin.pin_id);

        if (!send_sharpened_pin(second_to_third_sh_mem, pin)) {
            break;
        }
    }

    fputs("Second worker finished with error\n", stderr);
    exit(EXIT_FAILURE);
}

int main(void) {
    setup_signal_handlers(&shutdown_signal_handler);

    InitStatus res_f_s = init_shared_memory_info(&sh_mem_info_first_to_second,
                                                 FIRST_TO_SECOND_WORKER);
    if (res_f_s == SHARED_MEMORY_INIT_FAILURE) {
        return EXIT_FAILURE;
    }

    InitStatus res_s_t = init_shared_memory_info(&sh_mem_info_second_to_third,
                                                 SECOND_TO_THIRD_WORKER);
    if (res_s_t == SHARED_MEMORY_INIT_SUCCESS) {
        run_second_worker(sh_mem_info_first_to_second.sh_mem,
                          sh_mem_info_second_to_third.sh_mem);
        deinit_shared_memory_info(&sh_mem_info_second_to_third);
    }

    deinit_shared_memory_info(&sh_mem_info_first_to_second);
    return EXIT_SUCCESS;
}
