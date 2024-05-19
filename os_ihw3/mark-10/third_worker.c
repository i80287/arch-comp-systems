#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "workers_functions.h"

static QueueInfo queue_info;

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
            deinit_queue(&queue_info);
            break;
    }

    puts("Worker stopped working");
    exit(ret_status);
}

static int run_third_worker(mqd_t queue_id) {
    while (true) {
        Pin sharpened_pin;
        if (!wait_for_pin(queue_id, &sharpened_pin)) {
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
    return EXIT_FAILURE;
}

int main(void) {
    setup_signal_handlers(&shutdown_signal_handler);

    InitStatus res = init_queue(&queue_info, SECOND_TO_THIRD_WORKER);
    if (res == QUEUE_INIT_FAILURE) {
        return EXIT_FAILURE;
    }
    int ret = run_third_worker(queue_info.queue_id);
    deinit_queue(&queue_info);
    return ret;
}
