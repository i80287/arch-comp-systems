#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../util/parser.h"
#include "pin-tools.h"
#include "worker.h"

static void log_received_pin(Pin pin) {
    printf(
        "+-------------------------------------------------\n"
        "| Second worker received pin[pin_id=%d]\n"
        "| and started sharpening it...\n"
        "+-------------------------------------------------\n",
        pin.pin_id);
}

static void log_sharpened_pin(Pin pin) {
    printf(
        "+-------------------------------------------------\n"
        "| Second worker sharpened pin[pin_id=%d].\n"
        "+-------------------------------------------------\n",
        pin.pin_id);
}

static void log_sent_pin(Pin pin) {
    printf(
        "+-------------------------------------------------\n"
        "| Second worker sent sharpened\n"
        "| pin[pin_id=%d] to the third workers.\n"
        "+-------------------------------------------------\n",
        pin.pin_id);
}

static int start_runtime_loop(DualServerWorker worker) {
    int ret = EXIT_SUCCESS;
    for (bool is_running = true; is_running;) {
        Pin pin;
        if (!receive_not_crooked_pin(worker, &pin)) {
            ret = EXIT_FAILURE;
            break;
        }
        log_received_pin(pin);

        sharpen_pin(pin);
        log_sharpened_pin(pin);

        if (!send_sharpened_pin(worker, pin)) {
            ret = EXIT_FAILURE;
            break;
        }
        log_sent_pin(pin);
    }

    printf("Second worker is stopping...\n");
    return ret;
}

static int run_worker(const char* first_server_ip_address,
                      uint16_t first_server_port,
                      const char* second_server_ip_address,
                      uint16_t second_server_port) {
    DualServerWorker worker;
    if (!init_dual_server_worker(
            worker, first_server_ip_address, first_server_port,
            second_server_ip_address, second_server_port,
            SECOND_STAGE_WORKER)) {
        return EXIT_FAILURE;
    }

    print_dual_server_worker_info(worker);
    int ret = start_runtime_loop(worker);
    deinit_dual_server_worker(worker);
    return ret;
}

int main(int argc, const char* argv[]) {
    ParseResultWorker2 res = parse_args_worker_2(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_worker_2(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_worker(res.first_ip_address, res.first_port,
                      res.second_ip_address, res.second_port);
}
