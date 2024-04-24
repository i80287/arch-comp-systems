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

static int run_first_worker(int queue_id) {
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

        if (!send_produced_pin(queue_id, produced_pin)) {
            break;
        }
    }

    fputs("First worker finished with error\n", stderr);
    return EXIT_FAILURE;
}

int main(void) {
    setup_signal_handlers(&shutdown_signal_handler);

    InitStatus res = init_queue(&queue_info, FIRST_TO_SECOND_WORKER);
    if (res == QUEUE_INIT_FAILURE) {
        return EXIT_FAILURE;
    }
    int ret = run_first_worker(queue_info.queue_id);
    deinit_queue(&queue_info);
    return ret;
}
