#pragma once

#include <stdatomic.h>
#include <stdbool.h>

enum { TERMINATE_USING_SIGNAL = true };

enum message_type {
    EMPTY_MESSAGE = 0,
    NEW_NUMBER,
    SERVER_SHUTDOWN,
    CLIENT_SHUTDOWN,
    READY_TO_REMOVE_SEGMENT,
    SERVER_SENT_PROCESS_ID,
    CLIENT_SENT_PROCESS_ID,
    SERVER_GOT_PROCESS_ID,
    CLIENT_READY,
};

typedef struct {
    volatile _Atomic enum message_type type;
    int64_t rand_num;
} message_t;

enum {
    SHARED_MEMORY_KEY   = 0x42768284,
    SHARED_MEMORY_PERMS = 0666,
    SHARED_MEMORY_SIZE  = sizeof(message_t)
};
