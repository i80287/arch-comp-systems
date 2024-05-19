#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "worker.h"

enum {
    MAX_NUMBER_OF_FIRST_WORKERS  = 3,
    MAX_NUMBER_OF_SECOND_WORKERS = 5,
    MAX_NUMBER_OF_THIRD_WORKERS  = 2,

    MAX_WORKERS_PER_SERVER = MAX_NUMBER_OF_FIRST_WORKERS +
                             MAX_NUMBER_OF_SECOND_WORKERS +
                             MAX_NUMBER_OF_THIRD_WORKERS,
    MAX_CONNECTIONS_PER_SERVER = MAX_WORKERS_PER_SERVER,
};

typedef struct Server {
    volatile atomic_int sock_fd;
    volatile _Atomic struct sockaddr_in sock_addr;

    volatile _Atomic struct sockaddr_in first_workers_addrs[MAX_NUMBER_OF_FIRST_WORKERS];
    volatile atomic_int first_workers_fds[MAX_NUMBER_OF_FIRST_WORKERS];
    volatile atomic_size_t first_workers_arr_size;

    volatile _Atomic struct sockaddr_in second_workers_addrs[MAX_NUMBER_OF_SECOND_WORKERS];
    volatile atomic_int second_workers_fds[MAX_NUMBER_OF_SECOND_WORKERS];
    volatile atomic_size_t second_workers_arr_size;

    volatile _Atomic struct sockaddr_in third_workers_addrs[MAX_NUMBER_OF_THIRD_WORKERS];
    volatile atomic_int third_workers_fds[MAX_NUMBER_OF_THIRD_WORKERS];
    volatile atomic_size_t third_workers_arr_size;
} Server[1];

bool init_server(Server server, uint16_t server_port);

void deinit_server(Server server);

bool server_accept_worker(Server server, struct sockaddr_in* worker_addr,
                          WorkerType* type, size_t* insert_index);
