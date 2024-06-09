#pragma once

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>       // for EAGAIN, ECONNABORTED, ECONNREFUSED, ECON...
#include <math.h>        // for cos
#include <netinet/in.h>  // for sockaddr_in
#include <stdbool.h>     // for bool, false, true
#include <stdint.h>      // for uint32_t, uint16_t
#include <stdio.h>       // for printf, fprintf, perror, size_t, stderr
#include <stdlib.h>      // for rand
#include <sys/socket.h>  // for recv, sendto, socklen_t
#include <unistd.h>      // for sleep, ssize_t

#include "../util/config.h"  // for MIN_SLEEP_TIME, MAX_SLEEP_TIME
#include "net-config.h"      // for NET_BUFFER_SIZE, is_shutdown_message
#include "pin.h"             // for Pin

typedef struct Client {
    int client_sock_fd;
    ComponentType type;
    struct sockaddr_in server_broadcast_sock_addr;
} Client[1];

bool init_client(Client client, uint16_t server_port, ComponentType type);
void deinit_client(Client client);

static inline bool is_worker(const Client client) {
    switch (client->type) {
        case COMPONENT_TYPE_FIRST_STAGE_WORKER:
        case COMPONENT_TYPE_SECOND_STAGE_WORKER:
        case COMPONENT_TYPE_THIRD_STAGE_WORKER:
            return true;
        default:
            return false;
    }
}

bool client_should_stop(const Client client);
void print_sock_addr_info(const struct sockaddr* address, socklen_t sock_addr_len);
static inline void print_client_info(const Client client) {
    print_sock_addr_info((const struct sockaddr*)&client->server_broadcast_sock_addr,
                         sizeof(client->server_broadcast_sock_addr));
}
Pin receive_new_pin(void);
bool check_pin_crookness(Pin pin);
bool send_not_croocked_pin(const Client worker, Pin pin);
bool receive_not_crooked_pin(const Client worker, Pin* rec_pin);
void sharpen_pin(Pin pin);
bool send_sharpened_pin(const Client worker, Pin pin);
bool receive_sharpened_pin(const Client worker, Pin* rec_pin);
bool check_sharpened_pin_quality(Pin sharpened_pin);
