#pragma once

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../util/config.h"
#include "worker.h"

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
        char bytes[sizeof(pin)];
        Pin pin;
    } buffer;
    buffer.pin   = pin;
    bool success = sendto(sock_fd, buffer.bytes, sizeof(buffer.bytes), 0,
                          (const struct sockaddr*)sock_addr,
                          sizeof(*sock_addr)) == sizeof(buffer.bytes);
    if (!success) {
        perror("sendto");
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
        char bytes[sizeof(*rec_pin)];
        Pin pin;
    } buffer;
    struct sockaddr_in sender_address;
    socklen_t sender_address_len;
    sender_address_len = sizeof(sender_address);
    if (recvfrom(sock_fd, buffer.bytes, sizeof(buffer.bytes), 0,
                 (struct sockaddr*)&sender_address,
                 &sender_address_len) != sizeof(buffer.bytes)) {
        perror("recvfrom");
        return false;
    }

    bool wrong_sender =
        sender_address_len != sizeof(*sock_addr) ||
        sender_address.sin_addr.s_addr != sock_addr->sin_addr.s_addr ||
        sender_address.sin_port != sock_addr->sin_port;
    if (wrong_sender) {
        fputs("recvfrom wrong sender", stderr);
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
