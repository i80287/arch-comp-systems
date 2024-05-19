#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "workers_functions.h"

static QueueInfo queue_info_first_to_second;
static QueueInfo queue_info_second_to_third;

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
            deinit_queue(&queue_info_second_to_third);
            deinit_queue(&queue_info_first_to_second);
            break;
    }

    puts("Worker stopped working");
    exit(ret_status);
}

static int run_second_worker(mqd_t queue_id_first_to_second,
                             mqd_t queue_id_second_to_third) {
    while (true) {
        Pin pin;
        if (!wait_for_pin(queue_id_first_to_second, &pin)) {
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

        if (!send_sharpened_pin(queue_id_second_to_third, pin)) {
            break;
        }
    }

    fputs("Second worker finished with error\n", stderr);
    return EXIT_FAILURE;
}

int main(void) {
    setup_signal_handlers(&shutdown_signal_handler);

    InitStatus res_f_s =
        init_queue(&queue_info_first_to_second, FIRST_TO_SECOND_WORKER);
    if (res_f_s == QUEUE_INIT_FAILURE) {
        return EXIT_FAILURE;
    }

    InitStatus res_s_t =
        init_queue(&queue_info_second_to_third, SECOND_TO_THIRD_WORKER);
    int res = EXIT_FAILURE;
    if (res_s_t == QUEUE_INIT_SUCCESS) {
        res = run_second_worker(queue_info_first_to_second.queue_id,
                                queue_info_second_to_third.queue_id);
        deinit_queue(&queue_info_second_to_third);
    }

    deinit_queue(&queue_info_first_to_second);
    return res;
}
