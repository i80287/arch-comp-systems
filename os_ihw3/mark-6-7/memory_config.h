#ifndef MEMORY_CONFIG_H
#define MEMORY_CONFIG_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>

static const char FIRST_TO_SECOND_WORKER_SHARED_MEMORY[] = "1-2-sh-mem";
static const char SECOND_TO_THIRD_WORKER_SHARED_MEMORY[] = "2-3-sh-mem";

/// @brief Pin that workets pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;

enum { SH_MEM_BUFFER_SIZE = 4 };

typedef struct SharedMemory {
    volatile Pin buffer[SH_MEM_BUFFER_SIZE];
    sem_t buffer_access_sem;
    sem_t added_pins_sem;
    sem_t free_pins_sem;
    volatile size_t read_index;
    volatile size_t write_index;
} SharedMemory;

enum {
    SHARED_MEMORY_OPEN_FLAGS = O_CREAT | O_RDWR,
    SHARED_MEMORY_OPEN_PERMS_MODE = 0666,
    SHARED_MEMORY_SIZE  = sizeof(SharedMemory),
    MMAP_PROT = PROT_WRITE | PROT_READ,
    MMAP_FLAGS = MAP_SHARED,
    SEMAPHORE_INIT_IS_SHARED = true,
};

#endif
