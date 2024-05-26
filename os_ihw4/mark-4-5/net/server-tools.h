#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>       // for errno
#include <netinet/in.h>  // for sockaddr_in
#include <pthread.h>     // for pthread_mutex_t, pthread_mutex_lock, pthre...
#include <stdatomic.h>   // for atomic_int
#include <stdbool.h>     // for bool
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for size_t, perror
#include <stdlib.h>      // for rand

#include "net-config.h"

typedef struct ClientMetaInfo {
    char host[48];
    char port[16];
    char numeric_host[48];
    char numeric_port[16];
} ClientMetaInfo;

typedef struct Server {
    atomic_int sock_fd;
    struct sockaddr_in sock_addr;
} Server[1];

bool init_server(Server server, uint16_t server_port);
void deinit_server(Server server);

bool server_accept_client(Server server);
void send_shutdown_signal_to_one_client(const Server server, ClientType type, size_t index);
void send_shutdown_signal_to_first_workers(Server server);
void send_shutdown_signal_to_second_workers(Server server);
void send_shutdown_signal_to_third_workers(Server server);
void send_shutdown_signal_to_all_clients_of_type(Server server, ClientType type);
void send_shutdown_signal_to_all(Server server);

bool nonblocking_poll_workers_on_the_first_stage(Server server, PinsQueue pins_1_to_2);
bool nonblocking_poll_workers_on_the_second_stage(Server server, PinsQueue pins_1_to_2,
                                                  PinsQueue pins_2_to_3);
bool nonblocking_poll_workers_on_the_third_stage(Server server, PinsQueue pins_2_to_3);
