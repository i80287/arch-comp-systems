#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../util/config.h"
#include "net_config.h"
#include "pin.h"
#include "server.h"
#include "worker.h"

static inline void handle_errno(const char* cause) {
    switch (errno) {
        case ECONNABORTED:
        case ECONNRESET:
            printf(
                "+----------------------------+\n"
                "| Server stopped connections |\n"
                "+----------------------------+\n");
            break;
        default:
            perror(cause);
            break;
    }
}

static inline void handle_recvfrom_error(const char* bytes,
                                         ssize_t read_bytes) {
    if (read_bytes > 0) {
        if (is_shutdown_message(bytes, (size_t)read_bytes)) {
            printf(
                "+------------------------------------------+\n"
                "| Received shutdown signal from the server |\n"
                "+------------------------------------------+\n");
            return;
        }

        fprintf(stderr, "Read %zu unknown bytes: \"%.*s\"\n",
                (size_t)read_bytes, (int)read_bytes, bytes);
    }

    handle_errno("recvfrom");
}

static inline void handle_wrong_sender(
    const struct sockaddr_storage* sender_addr_storage,
    socklen_t sender_address_len) {
    fputs("Recieved data from the wrong sender:\n", stderr);
    print_sock_addr_info((const struct sockaddr*)sender_addr_storage,
                         sender_address_len);
}

static inline Pin receive_new_pin() {
    Pin pin = {.pin_id = rand()};
    return pin;
}

static inline bool check_pin_crookness(Pin pin) {
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) +
        MIN_SLEEP_TIME;
    sleep(sleep_time);

    uint32_t x = (uint32_t)pin.pin_id;
#if defined(__GNUC__)
    return __builtin_parity(x) & 1;
#else
    return x & 1;
#endif
}

static inline bool send_pin(int sock_fd,
                            const struct sockaddr_in* sock_addr, Pin pin) {
    union {
        char bytes[NET_BUFFER_SIZE];
        Pin pin;
    } buffer;
    buffer.pin   = pin;
    bool success = sendto(sock_fd, buffer.bytes, sizeof(pin), 0,
                          (const struct sockaddr*)sock_addr,
                          sizeof(*sock_addr)) == sizeof(pin);
    if (!success) {
        handle_errno("sendto");
    }
    return success;
}

static inline bool send_not_croocked_pin(Worker worker, Pin pin) {
    return send_pin(worker->worker_sock_fd, &worker->server_sock_addr,
                    pin);
}

static inline bool receive_pin(int sock_fd,
                               const struct sockaddr_in* sock_addr,
                               Pin* rec_pin) {
    union {
        char bytes[NET_BUFFER_SIZE];
        Pin pin;
    } buffer = {0};
    struct sockaddr_storage sender_addr_storage;
    socklen_t sender_address_len;
    sender_address_len = sizeof(sender_addr_storage);
    ssize_t read_bytes = recvfrom(
        sock_fd, buffer.bytes, sizeof(*rec_pin), 0,
        (struct sockaddr*)&sender_addr_storage, &sender_address_len);
    if (read_bytes != sizeof(*rec_pin)) {
        handle_recvfrom_error(buffer.bytes, read_bytes);
        return false;
    }

    bool wrong_sender = sender_address_len != sizeof(*sock_addr);
    if (wrong_sender) {
        handle_wrong_sender(&sender_addr_storage, sender_address_len);
        return false;
    }

    *rec_pin = buffer.pin;
    return true;
}

static inline bool receive_not_crooked_pin(Worker worker, Pin* rec_pin) {
    return receive_pin(worker->worker_sock_fd, &worker->server_sock_addr,
                       rec_pin);
}

static inline void sharpen_pin(Pin pin) {
    (void)pin;
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) +
        MIN_SLEEP_TIME;
    sleep(sleep_time);
}

static inline bool send_sharpened_pin(Worker worker, Pin pin) {
    return send_pin(worker->worker_sock_fd, &worker->server_sock_addr,
                    pin);
}

static inline bool receive_sharpened_pin(Worker worker, Pin* rec_pin) {
    return receive_pin(worker->worker_sock_fd, &worker->server_sock_addr,
                       rec_pin);
}

static inline bool check_sharpened_pin_quality(Pin sharpened_pin) {
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) +
        MIN_SLEEP_TIME;
    sleep(sleep_time);
    return cos(sharpened_pin.pin_id) >= 0;
}
