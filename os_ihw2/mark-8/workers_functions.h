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

static inline bool wait_for_pin(SharedMemory* sh_mem, Pin* pin) {
    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = 0,
        .sem_flg = 0,
    };

    buffer.sem_op = -1;
    if (semop(sh_mem->added_pins_sem_id, &buffer, 1) == -1) {
        perror("semop[added pins sem]");
        return false;
    }

    buffer.sem_op = -1;
    if (semop(sh_mem->buffer_access_sem_id, &buffer, 1) == -1) {
        perror("semop[buffer access sem]");
        return false;
    }

    size_t read_index  = sh_mem->read_index;
    *pin               = sh_mem->buffer[read_index];
    sh_mem->read_index = (read_index + 1) % SH_MEM_BUFFER_SIZE;
    buffer.sem_op = 1;
    if (semop(sh_mem->free_pins_sem_id, &buffer, 1) == -1) {
        perror("semop[free pins sem]");
        return false;
    }

    buffer.sem_op = 1;
    if (semop(sh_mem->buffer_access_sem_id, &buffer, 1) == -1) {
        perror("semop[buffer access sem]");
        return false;
    }

    return true;
}

static inline bool send_pin(SharedMemory* sh_mem, Pin pin,
                     const char* message_format) {
    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = 0,
        .sem_flg = 0,
    };

    buffer.sem_op = -1;
    if (semop(sh_mem->free_pins_sem_id, &buffer, 1) == -1) {
        perror("semop[free pins sem]");
        return false;
    }

    buffer.sem_op = -1;
    if (semop(sh_mem->buffer_access_sem_id, &buffer, 1) == -1) {
        perror("semop[buffer access sem]");
        return false;
    }

    size_t write_index          = sh_mem->write_index;
    sh_mem->buffer[write_index] = pin;
    sh_mem->write_index         = (write_index + 1) % SH_MEM_BUFFER_SIZE;
    printf(message_format, pin.pin_id);
    buffer.sem_op = 1;
    if (semop(sh_mem->added_pins_sem_id, &buffer, 1) == -1) {
        perror("semop[added pins sem]");
        return false;
    }

    buffer.sem_op = 1;
    if (semop(sh_mem->buffer_access_sem_id, &buffer, 1) == -1) {
        perror("semop[buffer access sem]");
        return false;
    }

    return true;
}

static inline bool send_produced_pin(SharedMemory* sh_mem, Pin pin) {
    return send_pin(
        sh_mem, pin,
        "+--------------------------------------------------------------\n"
        "| First worker sent not crooked\n"
        "| pin[pin_id=%d] to the second workers.\n"
        "+--------------------------------------------------------------\n");
}

static inline bool send_sharpened_pin(SharedMemory* sh_mem, Pin pin) {
    return send_pin(
        sh_mem, pin,
        "+--------------------------------------------------------------\n"
        "| Second worker sent sharpened\n"
        "| pin[pin_id=%d] to the third workers.\n"
        "+--------------------------------------------------------------\n");
}

#endif
