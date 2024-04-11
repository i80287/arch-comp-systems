#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

enum { TERMINATE_USING_SIGNAL = true };

enum message_type {
    EMPTY_MESSAGE = 0,
    NEW_NUMBER,
    SERVER_SHUTDOWN,
    CLIENT_SHUTDOWN,
    READY_TO_REMOVE_SEGMENT,
    SERVER_SENT_PROCESS_ID,
    CLIENT_SENT_PROCESS_ID,
    SERVER_GOT_PROCESS_ID
};

typedef struct {
    volatile _Atomic enum message_type type;
    int64_t rand_num;
} message_t;

const char* shared_object_name = "cl-sv-shared-memory";

enum {
    SHARED_MEMORY_OPEN_FLAGS = O_CREAT | O_RDWR,
    SHARED_MEMORY_OPEN_PERMS_MODE = 0666,
    SHARED_MEMORY_SIZE  = sizeof(message_t),
    MMAP_PROT = PROT_WRITE | PROT_READ,
    MMAP_FLAGS = MAP_SHARED,
};
