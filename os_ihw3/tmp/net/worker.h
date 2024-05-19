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

typedef struct SingleServerWorker {
    int worker_sock_fd;
    struct sockaddr_in server_sock_addr;
} SingleServerWorker[1];

typedef struct DualServerWorker {
    int worker_sock_fd;
    struct sockaddr_in first_server_sock_addr;
    struct sockaddr_in second_server_sock_addr;
} DualServerWorker[1];

bool init_single_server_worker(SingleServerWorker worker,
                               const char* server_ip,
                               uint16_t server_port,
                               WorkerType type);

void deinit_single_server_worker(SingleServerWorker worker);

void print_single_server_worker_info(SingleServerWorker worker);

bool init_dual_server_worker(DualServerWorker worker,
                             const char* first_server_ip,
                             uint16_t first_server_port,
                             const char* second_server_ip,
                             uint16_t second_server_port,
                             WorkerType type);

void deinit_dual_server_worker(DualServerWorker worker);

void print_dual_server_worker_info(DualServerWorker worker);
