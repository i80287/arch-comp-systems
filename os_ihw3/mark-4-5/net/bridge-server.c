#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../util/parser.h"
#include "../util/config.h"
#include "server.h"

/// @brief We use global variables so it can be accessed through
//   the signal handlers and another threads of this process.
static struct Server server              = {0};
static volatile bool is_acceptor_running = true;
static volatile bool is_poller_running   = true;

static void signal_handler(int sig) {
    is_acceptor_running = false;
    is_poller_running   = false;
    fprintf(stderr, "> Received signal %d\n", sig);
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

static bool check_socket(int sock_fd) {
    int error     = 0;
    socklen_t len = sizeof(error);
    int retval = getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (retval != 0) {
        fprintf(stderr, "> Error getting socket error code: %s\n",
                strerror(retval));
        return false;
    }
    if (error != 0) {
        fprintf(stderr, "> Socket error: %s\n", strerror(error));
        return false;
    }
    return true;
}

static bool poll_workers(int workers_fds[],
                         const WorkerMetainfo workers_info[],
                         volatile size_t* current_workers_online,
                         size_t max_number_of_workers, PinsQueue pins_q) {
    for (size_t i = 0;
         i < max_number_of_workers && *current_workers_online > 0 &&
         !pins_queue_full(pins_q);
         i++) {
        int worker_fd = workers_fds[i];
        if (worker_fd == -1) {
            continue;
        }
        if (!check_socket(worker_fd)) {
            workers_fds[i] = -1;
            close(worker_fd);
            (*current_workers_online)--;
            continue;
        }

        union {
            char bytes[NET_BUFFER_SIZE];
            Pin pin;
        } buffer = {0};
        if (recv(worker_fd, buffer.bytes, sizeof(Pin), MSG_DONTWAIT) !=
            sizeof(Pin)) {
            switch (errno) {
                case EAGAIN:
                    continue;
                case EINTR:
                case ECONNREFUSED:
                default:
                    perror("recv");
                    return false;
            }
        }

        printf(
            "> Received pin[pin_id=%d] from the worker[address=%s:%s]\n",
            buffer.pin.pin_id, workers_info[i].host_name,
            workers_info[i].port);
        bool res = pins_queue_try_put(pins_q, buffer.pin);
        assert(res);
    }

    return true;
}

static bool send_pins_to_workers(int workers_fds[],
                                 const WorkerMetainfo workers_info[],
                                 volatile size_t* current_workers_online,
                                 size_t max_number_of_workers,
                                 PinsQueue pins_q) {
    for (size_t i = 0;
         i < max_number_of_workers && *current_workers_online > 0 &&
         !pins_queue_empty(pins_q);
         i++) {
        int worker_fd = workers_fds[i];
        if (worker_fd == -1) {
            continue;
        }

        if (!check_socket(worker_fd)) {
            workers_fds[i] = -1;
            close(worker_fd);
            (*current_workers_online)--;
            continue;
        }

        union {
            char bytes[sizeof(Pin)];
            Pin pin;
        } buffer;
        Pin pin;
        bool res = pins_queue_try_pop(pins_q, &pin);
        assert(res);
        buffer.pin = pin;

        ssize_t sent_size =
            send(worker_fd, buffer.bytes, sizeof(Pin), MSG_DONTWAIT);

        if (sent_size != sizeof(Pin)) {
            switch (errno) {
                case EAGAIN:
                    continue;
                case EINTR:
                case ECONNREFUSED:
                default:
                    perror("send");
                    return false;
            }
        }

        printf("> Send pin[pid_id=%d] to the worker[address=%s:%s]\n",
               buffer.pin.pin_id, workers_info[i].host_name,
               workers_info[i].port);
    }

    return true;
}

static bool poll_workers_on_the_first_stage(PinsQueue pins_1_to_2) {
    if (server.first_workers_arr_size == 0 ||
        pins_queue_full(pins_1_to_2)) {
        return true;
    }

    // printf("> Starting to poll workers on the first stage\n");
    if (!server_lock_first_mutex(&server)) {
        return false;
    }
    bool res =
        poll_workers(server.first_workers_fds, server.first_workers_info,
                     &server.first_workers_arr_size,
                     MAX_NUMBER_OF_FIRST_WORKERS, pins_1_to_2);
    if (!server_unlock_first_mutex(&server)) {
        return false;
    }
    // printf("> Ended polling workers on the first stage\n");

    return res;
}

static bool poll_workers_on_the_second_stage(PinsQueue pins_1_to_2,
                                             PinsQueue pins_2_to_3) {
    if (server.second_workers_arr_size == 0 ||
        (pins_queue_full(pins_1_to_2) && pins_queue_empty(pins_2_to_3))) {
        return true;
    }

    // printf("> Starting to poll workers on the second stage\n");
    if (!server_lock_second_mutex(&server)) {
        return false;
    }
    bool res = send_pins_to_workers(
        server.second_workers_fds, server.second_workers_info,
        &server.second_workers_arr_size, MAX_NUMBER_OF_SECOND_WORKERS,
        pins_1_to_2);
    if (res) {
        poll_workers(server.second_workers_fds, server.second_workers_info,
                     &server.second_workers_arr_size,
                     MAX_NUMBER_OF_SECOND_WORKERS, pins_2_to_3);
    }
    if (!server_unlock_second_mutex(&server)) {
        return false;
    }
    // printf("> Ended polling workers on the second stage\n");

    return res;
}

static bool poll_workers_on_the_third_stage(PinsQueue pins_2_to_3) {
    if (server.third_workers_arr_size == 0 ||
        pins_queue_empty(pins_2_to_3)) {
        return true;
    }

    // printf("> Starting to poll workers on the third stage\n");
    if (!server_lock_third_mutex(&server)) {
        return false;
    }
    bool res = send_pins_to_workers(
        server.third_workers_fds, server.third_workers_info,
        &server.third_workers_arr_size, MAX_NUMBER_OF_THIRD_WORKERS,
        pins_2_to_3);
    if (!server_unlock_third_mutex(&server)) {
        return false;
    }
    // printf("> Ended polling workers on the third stage\n");

    return res;
}

static void* workers_poller(void* unused) {
    (void)unused;
    const struct timespec sleep_time = {
        .tv_sec  = 1,
        .tv_nsec = 500000000,
    };

    PinsQueue pins_1_to_2 = {0};
    PinsQueue pins_2_to_3 = {0};
    while (is_poller_running) {
        if (!poll_workers_on_the_first_stage(pins_1_to_2)) {
            assert(!"poll_workers_on_the_first_stage");
            break;
        }
        if (!poll_workers_on_the_second_stage(pins_1_to_2, pins_2_to_3)) {
            assert(!"poll_workers_on_the_second_stage");
            break;
        }
        if (!poll_workers_on_the_third_stage(pins_2_to_3)) {
            assert(!"poll_workers_on_the_third_stage");
            break;
        }
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {  // if not interrupted by the signal
                perror("nanosleep");
            }
            break;
        }
    }

    int32_t ret = is_poller_running ? EXIT_FAILURE : EXIT_SUCCESS;
    return (void*)(uintptr_t)(uint32_t)ret;
}

static void send_shutdown_signal_to_all(
    int sock_fds[], const WorkerMetainfo workers_info[],
    size_t max_array_size) {
    for (size_t i = 0; i < max_array_size; i++) {
        int sock_fd = sock_fds[i];
        if (sock_fd != -1) {
            send_shutdown_signal(sock_fd, &workers_info[i]);
        }
    }
}

static void send_shutdown_signals() {
    if (server_lock_first_mutex(&server)) {
        send_shutdown_signal_to_all(server.first_workers_fds,
                                           server.first_workers_info,
                                           MAX_NUMBER_OF_FIRST_WORKERS);
        server.first_workers_arr_size = 0;
        server_unlock_first_mutex(&server);
    }
    if (server_lock_second_mutex(&server)) {
        send_shutdown_signal_to_all(server.second_workers_fds,
                                           server.second_workers_info,
                                           MAX_NUMBER_OF_SECOND_WORKERS);
        server.second_workers_arr_size = 0;
        server_unlock_second_mutex(&server);
    }
    if (server_lock_third_mutex(&server)) {
        send_shutdown_signal_to_all(server.third_workers_fds,
                                           server.third_workers_info,
                                           MAX_NUMBER_OF_THIRD_WORKERS);
        server.third_workers_arr_size = 0;
        server_unlock_third_mutex(&server);
    }
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
        WorkerType type;
        size_t insert_index;
        server_accept_worker(&server, &type, &insert_index);
    }

    void* poll_ret = NULL;
    pthread_join(poll_thread, &poll_ret);
    send_shutdown_signals();
    printf("> Sent shutdown signals to all clients");
    sleep(MAX_SLEEP_TIME);
    return (int)(uintptr_t)poll_ret;
}

static int run_server(uint16_t server_port) {
    if (!init_server(&server, server_port)) {
        return EXIT_FAILURE;
    }

    int ret = start_runtime_loop(&server);
    deinit_server(&server);
    printf("> Deinitialized server resources\n");
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
