#include "resources.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int create_semaphore_or_increase(key_t sem_key,
                                        uint32_t sem_initial_value) {
    int sem_id        = semget(sem_key, 1, NEW_SEMAPHORE_OPEN_FLAGS);
    const bool failed = sem_id == -1 && errno != EEXIST;
    if (failed) {
        perror("semget");
        return sem_id;
    }

    const bool sem_already_exists = sem_id == -1;
    if (sem_already_exists) {
        sem_id = semget(sem_key, 1, EXISTING_SEMAPHORE_OPEN_FLAGS);
    }

    if (sem_id == -1) {
        perror("semget");
        return sem_id;
    }

    if (!sem_already_exists) {
        printf("Created new semaphore[sem_id=%d]\n", sem_id);
        if (semctl(sem_id, 0, SETVAL, sem_initial_value) == -1) {
            perror("semctl");
            if (semctl(sem_id, 0, IPC_RMID) == -1) {
                perror("shmctl");
            } else {
                printf(
                    "Could not init semaphore value. Deleted "
                    "semaphore[sem_id=%d]\n",
                    sem_id);
            }
            return -1;
        }
    } else {
        printf("Opened existing semaphore[sem_id=%d]\n", sem_id);
        struct sembuf buffer = {
            .sem_num = 0,
            .sem_op  = 1,
            .sem_flg = 0,
        };
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return -1;
        }
    }

    return sem_id;
}

static InitStatus init_queue_impl(QueueInfo* queue_info) {
    const TieType tie_type = queue_info->type;
    assert(tie_type <= SECOND_TO_THIRD_WORKER);
    const int queue_keys[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_QUEUE_KEY,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_QUEUE_KEY,
    };
    const int ref_cnt_sem_keys[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_REF_CNT_SEM_KEY,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_REF_CNT_SEM_KEY,
    };

    const int queue_key       = queue_keys[tie_type];
    const int ref_cnt_sem_key = ref_cnt_sem_keys[tie_type];

    int queue_id = msgget(queue_key, QUEUE_OPEN_FLAGS);
    if (queue_id == -1) {
        perror("msgget");
        return QUEUE_INIT_FAILURE;
    }

    queue_info->queue_id = queue_id;
    printf("Opened message queue[queue_id=%d]\n", queue_id);

    int sem_id                   = create_semaphore_or_increase(ref_cnt_sem_key,
                                              REF_CNT_SEM_DEFAULT_VALUE);
    queue_info->ref_count_sem_id = sem_id;
    return sem_id != -1 ? QUEUE_INIT_SUCCESS : QUEUE_INIT_FAILURE;
}

InitStatus init_queue(QueueInfo* queue_info, TieType tie_type) {
    assert(queue_info);
    queue_info->type             = tie_type;
    queue_info->queue_id         = -1;
    queue_info->ref_count_sem_id = -1;
    int ret                      = init_queue_impl(queue_info);
    if (ret == QUEUE_INIT_FAILURE) {
        deinit_queue(queue_info);
    }

    return ret;
}

static void delete_semaphore(int sem_id) {
    if (sem_id == -1) {
        return;
    }
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl");
    } else {
        printf("Deleted semaphore[sem_id=%d]\n", sem_id);
    }
}

static void delete_queue(int queue_id) {
    if (queue_id == -1) {
        return;
    }
    if (msgctl(queue_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
    } else {
        printf("Deleted queue[queue_id=%d]\n", queue_id);
    }
}

void deinit_queue(QueueInfo* queue_info) {
    assert(queue_info);

    const int queue_id = queue_info->queue_id;
    const int sem_id   = queue_info->ref_count_sem_id;
    if (sem_id == -1 || queue_id == -1) {
        return;
    }

    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = -1,
        .sem_flg = IPC_NOWAIT,
    };
    if (semop(sem_id, &buffer, 1) == -1) {
        bool counter_is_zero = errno == EAGAIN;
        if (counter_is_zero) {
            return;
        }

        perror("semop");
    }
    switch (semctl(sem_id, 0, GETVAL)) {
        case -1:
            perror("semctl");
            break;
        case 0:
            delete_semaphore(sem_id);
            delete_queue(queue_id);
            break;
        default:
            break;
    };
}
