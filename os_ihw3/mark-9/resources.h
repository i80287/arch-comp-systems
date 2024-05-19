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
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdatomic.h>

/// @brief Pin that workets pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;

enum {
    FIRST_TO_SECOND_WORKER_QUEUE_KEY = 8743490,
    SECOND_TO_THIRD_WORKER_QUEUE_KEY = 34549878,
    FIRST_TO_SECOND_WORKER_REF_CNT_SEM_KEY = 3482334,
    SECOND_TO_THIRD_WORKER_REF_CNT_SEM_KEY = 34534556,
    QUEUE_OPEN_FLAGS = 0666 | IPC_CREAT,
    NEW_SEMAPHORE_OPEN_FLAGS = 0666 | IPC_CREAT | IPC_EXCL,
    EXISTING_SEMAPHORE_OPEN_FLAGS = 0666,
    REF_CNT_SEM_DEFAULT_VALUE = 1,
};

typedef enum TieType {
    FIRST_TO_SECOND_WORKER = 0,
    SECOND_TO_THIRD_WORKER = 1,
} TieType;

typedef struct QueueInfo {
    int queue_id;
    int ref_count_sem_id;
    TieType type;
} QueueInfo;

typedef enum InitStatus {
    QUEUE_INIT_SUCCESS,
    QUEUE_INIT_FAILURE,
} InitStatus;

InitStatus init_queue(QueueInfo* res, TieType tie_type);

void deinit_queue(QueueInfo* res);

#endif
