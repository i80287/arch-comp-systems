#ifndef WORKER_CONFIG
#define WORKER_CONFIG

#include "resources.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>

enum {
    MIN_SLEEP_TIME = 1,
    MAX_SLEEP_TIME = 9,
};

static inline void setup_signal_handlers(void (*handler)(int)) {
    signal(SIGINT, handler);
    signal(SIGKILL, handler);
    signal(SIGABRT, handler);
    signal(SIGTERM, handler);
}

static inline bool check_pin_crookness(Pin produced_pin) {
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) + MIN_SLEEP_TIME;
    sleep(sleep_time);

    uint32_t x = (uint32_t)produced_pin.pin_id;
#if defined(__GNUC__)
    return __builtin_parity(x) & 1;
#else
    return x & 1;
#endif
}

static inline void sharpen_pin(Pin produced_pin) {
    (void)produced_pin;
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) + MIN_SLEEP_TIME;
    sleep(sleep_time);
    return;
}

static inline bool check_sharpened_pin_quality(Pin sharpened_pin) {
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) + MIN_SLEEP_TIME;
    sleep(sleep_time);
    return cos(sharpened_pin.pin_id) >= 0;
}

static inline bool wait_for_pin(int queue_id, Pin* pin) {
    // According to man msgrcv
    struct msgbuf {
        long mtype;
        Pin pin;
    };

    struct msgbuf buffer = {
        .mtype = 1,
        .pin = { .pin_id = 0 },
    };

    if (msgrcv(queue_id, &buffer, sizeof(*pin), 1, 0) != sizeof(*pin)) {
        perror("msgrcv");
        return false;
    }

    *pin = buffer.pin;
    return true;
}

static inline bool send_pin(int queue_id, Pin pin,
                     const char* message_format) {
    // According to man msgsnd
    struct msgbuf {
        long mtype;
        Pin pin;
    };

    struct msgbuf buffer = {
        .mtype = 1,
        .pin = pin
    };
    if (msgsnd(queue_id, &buffer, sizeof(pin), 0) == -1) {
        perror("msgsnd");
        return false;
    }
    printf(message_format, pin.pin_id);

    return true;
}

static inline bool send_produced_pin(int queue_id, Pin pin) {
    return send_pin(
        queue_id, pin,
        "+--------------------------------------------------------------\n"
        "| First worker sent not crooked\n"
        "| pin[pin_id=%d] to the second workers.\n"
        "+--------------------------------------------------------------\n");
}

static inline bool send_sharpened_pin(int queue_id, Pin pin) {
    return send_pin(
        queue_id, pin,
        "+--------------------------------------------------------------\n"
        "| Second worker sent sharpened\n"
        "| pin[pin_id=%d] to the third workers.\n"
        "+--------------------------------------------------------------\n");
}

#endif
