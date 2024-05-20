#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pin.h"
#include "net_config.h"
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

typedef struct WorkerMetainfo {
    char host_name[48];
    char port[16];
} WorkerMetainfo;

typedef struct Server {
    atomic_int sock_fd;
    struct sockaddr_in sock_addr;

    pthread_mutex_t first_workers_mutex;
    struct sockaddr_in first_workers_addrs[MAX_NUMBER_OF_FIRST_WORKERS];
    int first_workers_fds[MAX_NUMBER_OF_FIRST_WORKERS];
    struct WorkerMetainfo first_workers_info[MAX_NUMBER_OF_FIRST_WORKERS];
    volatile size_t first_workers_arr_size;

    pthread_mutex_t second_workers_mutex;
    struct sockaddr_in second_workers_addrs[MAX_NUMBER_OF_SECOND_WORKERS];
    int second_workers_fds[MAX_NUMBER_OF_SECOND_WORKERS];
    struct WorkerMetainfo
        second_workers_info[MAX_NUMBER_OF_SECOND_WORKERS];
    volatile size_t second_workers_arr_size;

    pthread_mutex_t third_workers_mutex;
    struct sockaddr_in third_workers_addrs[MAX_NUMBER_OF_THIRD_WORKERS];
    int third_workers_fds[MAX_NUMBER_OF_THIRD_WORKERS];
    struct WorkerMetainfo third_workers_info[MAX_NUMBER_OF_THIRD_WORKERS];
    volatile size_t third_workers_arr_size;
} Server[1];

bool init_server(Server server, uint16_t server_port);

void deinit_server(Server server);

void send_shutdown_signal(int sock_fd, const WorkerMetainfo* info);

bool server_accept_worker(Server server, WorkerType* type,
                          size_t* insert_index);

static inline bool server_lock_mutex(pthread_mutex_t* mutex) {
    int err = pthread_mutex_lock(mutex);
    if (err != 0) {
        errno = err;
        perror("pthread_mutex_lock");
    }
    return err == 0;
}

static inline bool server_unlock_mutex(pthread_mutex_t* mutex) {
    int err = pthread_mutex_unlock(mutex);
    if (err != 0) {
        errno = err;
        perror("pthread_mutex_unlock");
    }
    return err == 0;
}

static inline bool server_lock_first_mutex(Server server) {
    return server_lock_mutex(&server->first_workers_mutex);
}

static inline bool server_unlock_first_mutex(Server server) {
    return server_unlock_mutex(&server->first_workers_mutex);
}

static inline bool server_lock_second_mutex(Server server) {
    return server_lock_mutex(&server->second_workers_mutex);
}

static inline bool server_unlock_second_mutex(Server server) {
    return server_unlock_mutex(&server->second_workers_mutex);
}

static inline bool server_lock_third_mutex(Server server) {
    return server_lock_mutex(&server->third_workers_mutex);
}

static inline bool server_unlock_third_mutex(Server server) {
    return server_unlock_mutex(&server->third_workers_mutex);
}
