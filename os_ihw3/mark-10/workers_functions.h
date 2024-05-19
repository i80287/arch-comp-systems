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

static inline bool wait_for_pin(mqd_t queue_id, Pin* pin) {
    union {
        char bytes[QUEUE_MESSAGE_SIZE + 1];
        Pin pin;
    } buffer = {0};

    if (mq_receive(queue_id, buffer.bytes, QUEUE_MESSAGE_SIZE, 0) != QUEUE_MESSAGE_SIZE) {
        perror("mq_receive");
        return false;
    }

    *pin = buffer.pin;
    return true;
}

static inline bool send_pin(mqd_t queue_id, Pin pin,
                     const char* message_format) {
    union {
        char bytes[QUEUE_MESSAGE_SIZE + 1];
        Pin pin;
    } buffer;
    buffer.pin = pin;

    if (mq_send(queue_id, buffer.bytes, QUEUE_MESSAGE_SIZE, 0) == -1) {
        perror("mq_send");
        return false;
    }
    printf(message_format, pin.pin_id);

    return true;
}

static inline bool send_produced_pin(mqd_t queue_id, Pin pin) {
    return send_pin(
        queue_id, pin,
        "+--------------------------------------------------------------\n"
        "| First worker sent not crooked\n"
        "| pin[pin_id=%d] to the second workers.\n"
        "+--------------------------------------------------------------\n");
}

static inline bool send_sharpened_pin(mqd_t queue_id, Pin pin) {
    return send_pin(
        queue_id, pin,
        "+--------------------------------------------------------------\n"
        "| Second worker sent sharpened\n"
        "| pin[pin_id=%d] to the third workers.\n"
        "+--------------------------------------------------------------\n");
}

#endif
