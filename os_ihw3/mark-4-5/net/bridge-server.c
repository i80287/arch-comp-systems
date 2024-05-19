#ifndef _POSIX_C_SOURCE
// For nanosleep
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "parser.h"
#include "server.h"

/// @brief We use global variables so it can be accessed through
//   the signal handlers and another threads of this process.
static struct Server server              = {0};
static volatile bool is_acceptor_running = true;
static volatile bool is_poller_running   = true;

static void signal_handler(int sig) {
    switch (sig) {
        case SIGABRT:
        case SIGINT:
        case SIGTERM:
        case SIGSEGV:
        case SIGQUIT:
        case SIGKILL:
            is_acceptor_running = false;
            is_poller_running   = false;
            break;
    }
}

static void setup_signal_handler() {
    const int handled_signals[] = {
        SIGABRT, SIGINT, SIGTERM, SIGSEGV, SIGQUIT, SIGKILL,
    };
    for (size_t i = 0;
         i < sizeof(handled_signals) / sizeof(handled_signals[0]); i++) {
        signal(handled_signals[i], signal_handler);
    }
}

static size_t poll_workers(
    const volatile atomic_int workers_fds[],
    const volatile _Atomic struct sockaddr_in workers_addrs[],
    size_t max_number_of_workers, Pin pins_buffer[]) {
    size_t received_pins_size = 0;
    for (size_t i = 0; i < max_number_of_workers; i++) {
        int worker_fd = workers_fds[i];
        if (worker_fd == -1) {
            continue;
        }

        union {
            char bytes[sizeof(Pin)];
            Pin pin;
        } buffer = {0};
        if (recv(worker_fd, buffer.bytes, sizeof(buffer.bytes),
                 MSG_DONTWAIT) != sizeof(buffer.bytes)) {
            switch (errno) {
                case EAGAIN:
                    continue;
                case EINTR:
                case ECONNREFUSED:
                default:
                    perror("recv");
                    break;
            }
        }

        pins_buffer[received_pins_size++] = buffer.pin;
    }

    return received_pins_size;
}

static void send_pins_to_workers(
    const volatile atomic_int* workers_fds,
    const volatile _Atomic struct sockaddr_in workers_addrs[],
    size_t max_number_of_workers, Pin pins[], size_t pins_size) {
    for (size_t pin_index = 0; pin_index < pins_size;) {
        for (size_t i = 0; i < max_number_of_workers; i++) {
            int worker_fd = workers_fds[i];
            if (worker_fd == -1) {
                continue;
            }

            union {
                char bytes[sizeof(Pin)];
                Pin pin;
            } buffer;
            buffer.pin = pins[pin_index];
            if (send(worker_fd, buffer.bytes, sizeof(buffer.bytes),
                     MSG_DONTWAIT) != buffer.bytes) {
                switch (errno) {
                    case EAGAIN:
                        continue;
                    case EINTR:
                    case ECONNREFUSED:
                    default:
                        perror("recv");
                        break;
                }
            }

            printf("Send pin[pid_id=%d] to the ") pin_index++;
            if (pin_index == pins_size) {
                return;
            }
        }
    }
}

static void* workers_poller(void*) {
    const struct timespec sleep_time = {
        .tv_sec  = 0,
        .tv_nsec = 20000,
    };

    Pin pins[MAX_WORKERS_PER_SERVER] = {0};
    while (is_poller_running) {
        size_t recv_pins_size = poll_workers(
            server.first_workers_fds, server.first_workers_addrs,
            MAX_NUMBER_OF_FIRST_WORKERS, pins);
        if (recv_pins_size != 0) {
            send_pins_to_workers(
                server.second_workers_fds, server.second_workers_addrs,
                MAX_NUMBER_OF_SECOND_WORKERS, pins, recv_pins_size);
        }

        recv_pins_size = poll_workers(server.second_workers_fds,
                                      server.second_workers_addrs,
                                      MAX_NUMBER_OF_SECOND_WORKERS, pins);
        if (recv_pins_size != 0) {
            send_pins_to_workers(
                server.third_workers_fds, server.third_workers_addrs,
                MAX_NUMBER_OF_THIRD_WORKERS, pins, recv_pins_size);
        }

        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno == EINTR) {  // interrupted by the signal
                break;
            }
            perror("nanosleep");
        }
    }

    return NULL;
}

static int start_runtime_loop() {
    pthread_t poll_thread = 0;
    int ret = pthread_create(&poll_thread, NULL, &workers_poller, NULL);
    if (ret != 0) {
        errno = ret;
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    while (is_acceptor_running) {
        struct sockaddr_in worker_addr;
        WorkerType type;
        size_t insert_index;
        server_accept_worker(&server, &worker_addr, &type, &insert_index);
    }

    pthread_join(poll_thread, NULL);
}

static int run_server(uint16_t server_port) {
    if (!init_server(&server, server_port)) {
        return EXIT_FAILURE;
    }

    int ret = start_runtime_loop(&server);
    wait_for_children();
    deinit_server(&server);
    return ret;
}

int main(int argc, const char* argv[]) {
    setup_signal_handler();

    ParseResultServer res = parse_args_server(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_server(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(res.port);
}
