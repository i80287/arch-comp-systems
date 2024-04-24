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
#include <mqueue.h>
#include <semaphore.h>
#include <stdatomic.h>

/// @brief Pin that workets pass to each other.
typedef struct Pin {
    int pin_id;
} Pin;

static const char FIRST_TO_SECOND_WORKER_QUEUE_NAME[] = "/1-2-queue";
static const char SECOND_TO_THIRD_WORKER_QUEUE_NAME[] = "/2-3-queue";

static const char FIRST_TO_SECOND_WORKER_REF_CNT_SEM_NAME[] = "1-2-ref-cnt-sem";
static const char SECOND_TO_THIRD_WORKER_REF_CNT_SEM_NAME[] = "2-3-ref-cnt-sem";

enum {
    QUEUE_OPEN_FLAGS = O_RDWR | O_CREAT,
    QUEUE_OPEN_PERMS = 0666,
    SEMAPHORE_OPEN_FLAGS = O_CREAT,
    SEMAPHORE_OPEN_PERMS_MODE = 0666,
    REF_CNT_SEM_DEFAULT_VALUE = 1,
    MAX_QUEUE_MESSAGES = 4,
    QUEUE_MESSAGE_SIZE = sizeof(Pin),
};

typedef enum TieType {
    FIRST_TO_SECOND_WORKER = 0,
    SECOND_TO_THIRD_WORKER = 1,
} TieType;

typedef struct QueueInfo {
    mqd_t queue_id;
    sem_t* ref_count_sem;
    TieType type;
} QueueInfo;

typedef enum InitStatus {
    QUEUE_INIT_SUCCESS,
    QUEUE_INIT_FAILURE,
} InitStatus;

InitStatus init_queue(QueueInfo* res, TieType tie_type);

void deinit_queue(QueueInfo* res);

#endif
