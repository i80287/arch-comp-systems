#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

/// @brief Pin that workers pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;

typedef enum WorkerType {
    FIRST_STAGE_WORKER,
    SECOND_STAGE_WORKER,
    THIRD_STAGE_WORKER,
} WorkerType;

typedef struct Worker {
    int worker_sock_fd;
    WorkerType type;
    struct sockaddr_in server_sock_addr;
} Worker[1];

bool init_worker(Worker worker, const char* server_ip,
                 uint16_t server_port, WorkerType type);

void deinit_worker(Worker worker);

void print_worker_info(Worker worker);
