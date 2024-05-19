#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../util/parser.h"
#include "pin-tools.h"
#include "worker.h"

static void log_received_pin(Pin pin) {
    printf(
        "+----------------------------------------------\n"
        "| First worker received pin[pin_id=%d]\n"
        "| and started checking it's crookness...\n"
        "+----------------------------------------------\n",
        pin.pin_id);
}

static void log_checked_pin(Pin pin, bool check_result) {
    printf(
        "+----------------------------------------------\n"
        "| First worker decision:\n"
        "| pin[pin_id=%d] is%s crooked.\n"
        "+----------------------------------------------\n",
        pin.pin_id, (check_result ? " not" : ""));
}

static void log_sent_pin(Pin pin) {
    printf(
        "+----------------------------------------------\n"
        "| First worker sent not crooked\n"
        "| pin[pin_id=%d] to the second stage workers.\n"
        "+----------------------------------------------\n",
        pin.pin_id);
}

static int start_runtime_loop(SingleServerWorker worker) {
    int ret = EXIT_SUCCESS;
    for (bool is_running = true; is_running;) {
        const Pin pin = receive_new_pin();
        log_received_pin(pin);
        bool is_ok = check_pin_crookness(pin);
        log_checked_pin(pin, is_ok);
        if (!is_ok) {
            continue;
        }

        if (!send_not_croocked_pin(worker, pin)) {
            is_running = false;
            ret        = EXIT_FAILURE;
            break;
        }
        log_sent_pin(pin);
    }

    printf("First worker is stopping...\n");
    return ret;
}

static int run_worker(const char* server_ip_address,
                      uint16_t server_port) {
    SingleServerWorker worker;
    if (!init_single_server_worker(worker, server_ip_address, server_port,
                                   FIRST_STAGE_WORKER)) {
        return EXIT_FAILURE;
    }

    print_single_server_worker_info(worker);
    int ret = start_runtime_loop(worker);
    deinit_single_server_worker(worker);
    return ret;
}

int main(int argc, const char* argv[]) {
    ParseResultWorker1 res = parse_args_worker_1(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_worker_1(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_worker(res.ip_address, res.port);
}
