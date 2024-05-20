#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <assert.h>      // for assert
#include <errno.h>       // for errno, EAGAIN, ECONNABORTED, ECON...
#include <pthread.h>     // for pthread_create, pthread_join, pth...
#include <signal.h>      // for size_t, signal, SIGABRT, SIGINT
#include <stdbool.h>     // for bool, false, true
#include <stdint.h>      // for uintptr_t, int32_t, uint16_t, uin...
#include <stdio.h>       // for fprintf, perror, printf, stderr
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>      // for strerror
#include <sys/socket.h>  // for MSG_DONTWAIT, getsockopt, recv, send, SOL_SOCKET, SO_ERROR
#include <time.h>        // for nanosleep
#include <unistd.h>      // for close, sleep, ssize_t

#include "../util/config.h"  // for MAX_SLEEP_TIME
#include "../util/parser.h"  // for parse_args_server, print_invalid_...
#include "client-tools.h"
#include "net-config.h"    // for NET_BUFFER_SIZE
#include "pin.h"           // for Pin, PinsQueue, pins_queue_empty
#include "server-tools.h"  // for Server, WorkerMetainfo, MAX_NUMBE...

/// @brief We use global variables so it can be accessed through
//   the signal handlers and another threads of this process.
static struct Server server              = {0};
static volatile bool is_acceptor_running = true;
static volatile bool is_poller_running   = true;
static volatile bool is_logger_running   = true;

static void stop_all_threads() {
    is_acceptor_running = false;
    is_poller_running   = false;
    is_logger_running   = false;
}
static void signal_handler(int sig) {
    stop_all_threads();
    fprintf(stderr, "> Received signal %d\n", sig);
}
static void setup_signal_handler() {
    const int handled_signals[] = {
        SIGABRT, SIGINT, SIGTERM, SIGSEGV, SIGQUIT, SIGKILL,
    };
    for (size_t i = 0; i < sizeof(handled_signals) / sizeof(handled_signals[0]); i++) {
        signal(handled_signals[i], signal_handler);
    }
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
        if (!nonblocking_poll_workers_on_the_first_stage(&server, pins_1_to_2)) {
            fprintf(stderr, "> Could not poll workers on the first stage\n");
            break;
        }
        if (!nonblocking_poll_workers_on_the_second_stage(&server, pins_1_to_2, pins_2_to_3)) {
            fprintf(stderr, "> Could not poll workers on the second stage\n");
            break;
        }
        if (!nonblocking_poll_workers_on_the_third_stage(&server, pins_2_to_3)) {
            fprintf(stderr, "> Could not poll workers on the third stage\n");
            break;
        }
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {  // if not interrupted by the signal
                perror("nanosleep");
            }
            break;
        }
    }

    int32_t ret       = is_poller_running ? EXIT_FAILURE : EXIT_SUCCESS;
    is_poller_running = false;
    return (void*)(uintptr_t)(uint32_t)ret;
}

static void* logs_sender(void* unused) {
    (void)unused;
    const struct timespec sleep_time = {
        .tv_sec  = 1,
        .tv_nsec = 500000000,
    };

    ServerLog log = {0};
    while (is_logger_running) {
        if (!dequeue_log(&server, &log)) {
            fprintf(stderr, "> Could not get next log\n");
            break;
        }

        send_server_log(&server, &log);
        if (nanosleep(&sleep_time, NULL) == -1) {
            if (errno != EINTR) {  // if not interrupted by the signal
                perror("nanosleep");
            }
            break;
        }
    }

    int32_t ret       = is_logger_running ? EXIT_FAILURE : EXIT_SUCCESS;
    is_poller_running = false;
    return (void*)(uintptr_t)(uint32_t)ret;
}

static bool create_polling_thread(pthread_t* pthread_id) {
    int ret = pthread_create(pthread_id, NULL, &workers_poller, NULL);
    if (ret != 0) {
        stop_all_threads();
        errno = ret;
        perror("pthread_create");
        return false;
    }
    return true;
}

static bool create_logging_thread(pthread_t* pthread_id) {
    int ret = pthread_create(pthread_id, NULL, &logs_sender, NULL);
    if (ret != 0) {
        stop_all_threads();
        errno = ret;
        perror("pthread_create");
        return false;
    }
    return true;
}

static int join_thread(pthread_t pthread_id) {
    void* poll_ret       = NULL;
    int pthread_join_ret = pthread_join(pthread_id, &poll_ret);
    if (pthread_join_ret != 0) {
        errno = pthread_join_ret;
        perror("pthread_join");
    }
    return (int)(uintptr_t)poll_ret;
}

static int start_runtime_loop() {
    pthread_t poll_thread;
    if (!create_polling_thread(&poll_thread)) {
        return EXIT_FAILURE;
    }
    printf("> Started polling thread\n");

    pthread_t logs_thread;
    if (!create_logging_thread(&logs_thread)) {
        pthread_detach(poll_thread);
        return EXIT_FAILURE;
    }
    printf("> Started logging thread\n");

    ServerLog log = {0};
    printf("> Server ready to accept connections\n");
    while (is_acceptor_running) {
        ClientType type;
        size_t insert_index;
        server_accept_client(&server, &type, &insert_index);
        if (!nonblocking_enqueue_log(&server, &log)) {
            fprintf(stderr, "> Logs queue if full. Can't add new log to the queue\n");
        }
    }

    int ret1 = join_thread(logs_thread);
    int ret2 = join_thread(poll_thread);

    printf("Started sending shutdown signals to all clients\n");
    send_shutdown_signal_to_all(&server);
    printf(
        "> Sent shutdown signals to all clients\n"
        "> Started waiting for %u seconds before closing the sockets\n",
        (uint32_t)MAX_SLEEP_TIME);
    sleep(MAX_SLEEP_TIME);

    return ret1 | ret2;
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
