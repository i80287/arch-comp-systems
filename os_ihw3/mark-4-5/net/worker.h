#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "net-config.h"
#include "pin.h"

typedef enum WorkerType {
    FIRST_STAGE_WORKER  = 1,
    SECOND_STAGE_WORKER = 2,
    THIRD_STAGE_WORKER  = 3,
} WorkerType;

static inline const char* worker_type_to_string(WorkerType type) {
    switch (type) {
        case FIRST_STAGE_WORKER:
            return "first stage worker";
        case SECOND_STAGE_WORKER:
            return "second stage worker";
        case THIRD_STAGE_WORKER:
            return "third stage worker";
        default:
            return "unknown stage worker";
    }
}

typedef struct Worker {
    int worker_sock_fd;
    WorkerType type;
    struct sockaddr_in server_sock_addr;
} Worker[1];

bool init_worker(Worker worker, const char* server_ip,
                 uint16_t server_port, WorkerType type);

void deinit_worker(Worker worker);

void print_sock_addr_info(const struct sockaddr* address,
                          socklen_t sock_addr_len);

static inline void print_worker_info(Worker worker) {
    print_sock_addr_info((const struct sockaddr*)&worker->server_sock_addr,
                         sizeof(worker->server_sock_addr));
}

bool worker_should_stop(const Worker worker);

static inline void worker_handle_shutdown_signal() {
    printf("Received shutdown signal from the server\n");
}
