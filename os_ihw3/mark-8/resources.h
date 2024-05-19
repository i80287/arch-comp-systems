#ifndef RESOURCES_H
#define RESOURCES_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdatomic.h>

/// @brief Pin that workets pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;

enum { SH_MEM_BUFFER_SIZE = 4 };

typedef struct SharedMemory {
    volatile Pin buffer[SH_MEM_BUFFER_SIZE];
    volatile int buffer_access_sem_id;
    volatile int added_pins_sem_id;
    volatile int free_pins_sem_id;
    volatile size_t read_index;
    volatile size_t write_index;
    volatile atomic_int sh_mem_ref_counter;
} SharedMemory;

enum {
    FIRST_TO_SECOND_WORKER_SHARED_MEMORY_KEY = 8743490,
    SECOND_TO_THIRD_WORKER_SHARED_MEMORY_KEY = 34549878,
    FIRST_TO_SECOND_WORKER_ACCESS_SEM_KEY = 3482334,
    FIRST_TO_SECOND_WORKER_ADD_SEM_KEY = 34534556,
    FIRST_TO_SECOND_WORKER_FREE_SEM_KEY = 4350928,
    SECOND_TO_THIRD_WORKER_ACCESS_SEM_KEY = 2301489,
    SECOND_TO_THIRD_WORKER_ADD_SEM_KEY = 2340893,
    SECOND_TO_THIRD_WORKER_FREE_SEM_KEY = 23464923,
    NEW_SHARED_MEMORY_FLAGS = 0666 | IPC_CREAT | IPC_EXCL,
    EXISTING_SHARED_MEMORY_FLAGS = 0666,
    SHARED_MEMORY_SIZE  = sizeof(SharedMemory),
    NEW_SEMAPHORE_OPEN_FLAGS = 0666 | IPC_CREAT | IPC_EXCL,
    EXISTING_SEMAPHORE_OPEN_FLAGS = 0666,
    ACCESS_SEM_DEFAULT_VALUE = 1,
    ADD_SEM_DEFAULT_VALUE = 0,
    FREE_SEM_DEFAULT_VALUE = SH_MEM_BUFFER_SIZE,
};

typedef enum TieType {
    FIRST_TO_SECOND_WORKER = 0,
    SECOND_TO_THIRD_WORKER = 1,
} TieType;

typedef struct SharedMemoryInfo {
    int shared_mem_id;
    SharedMemory* sh_mem;
    TieType type;
} SharedMemoryInfo;

typedef enum InitStatus {
    SHARED_MEMORY_INIT_SUCCESS,
    SHARED_MEMORY_INIT_FAILURE,
} InitStatus;

InitStatus init_shared_memory_info(SharedMemoryInfo* res, TieType tie_type);

void deinit_shared_memory_info(SharedMemoryInfo* res);

#endif
