#pragma once

#include <stdatomic.h>

enum message_type { EMPTY_MESSAGE = 0, NEW_NUMBER, SERVER_SHUTDOWN_SIGNAL, CLIENT_SHUTDOWN_SIGNAL, READY_TO_REMOVE_SEGMENT };

typedef struct {
    volatile _Atomic enum message_type type;
    uint64_t rand_num;
} message_t;

enum {
    SHARED_MEMORY_KEY   = 0x42768284,
    SHARED_MEMORY_PERMS = 0666,
    SHARED_MEMORY_SIZE  = sizeof(message_t)
};
