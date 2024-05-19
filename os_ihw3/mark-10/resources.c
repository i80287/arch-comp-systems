#include "resources.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

static const char* get_queue_name(TieType tie_type) {
    assert(tie_type <= SECOND_TO_THIRD_WORKER);
    const char* queue_names[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_QUEUE_NAME,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_QUEUE_NAME,
    };
    return queue_names[tie_type];
}

static const char* get_semaphore_name(TieType tie_type) {
    assert(tie_type <= SECOND_TO_THIRD_WORKER);
    const char* ref_cnt_sem_names[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_REF_CNT_SEM_NAME,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_REF_CNT_SEM_NAME,
    };
    return ref_cnt_sem_names[tie_type];
}

static InitStatus init_queue_impl(QueueInfo* queue_info) {
    const char* queue_name       = get_queue_name(queue_info->type);
    const char* ref_cnt_sem_name = get_semaphore_name(queue_info->type);

    struct mq_attr mqattrs = {0};
    mqattrs.mq_maxmsg      = MAX_QUEUE_MESSAGES;
    mqattrs.mq_msgsize     = QUEUE_MESSAGE_SIZE;
    mqd_t queue_id =
        mq_open(queue_name, QUEUE_OPEN_FLAGS, QUEUE_OPEN_PERMS, &mqattrs);
    if (queue_id == (mqd_t)-1) {
        perror("mq_open");
        return QUEUE_INIT_FAILURE;
    }

    queue_info->queue_id = queue_id;
    printf("Opened message queue[queue_id=%d,queue_name=%s]\n", queue_id,
           queue_name);

    sem_t* ref_cnt_sem =
        sem_open(ref_cnt_sem_name, NEW_SEMAPHORE_OPEN_FLAGS,
                 SEMAPHORE_OPEN_PERMS_MODE, REF_CNT_SEM_DEFAULT_VALUE);
    bool failed = ref_cnt_sem == SEM_FAILED && errno != EEXIST;
    if (failed) {
        perror("sem_open");
        return QUEUE_INIT_FAILURE;
    }
    const bool sem_already_exists = ref_cnt_sem == SEM_FAILED;
    if (sem_already_exists) {
        ref_cnt_sem = sem_open(ref_cnt_sem_name, EXISTING_SEMAPHORE_OPEN_FLAGS);
        if (sem_post(ref_cnt_sem) == -1) {
            perror("sem_post");
        }
    }
    if (ref_cnt_sem == SEM_FAILED) {
        perror("sem_open");
        return QUEUE_INIT_FAILURE;
    }

    const char* fmt = sem_already_exists 
    ? "Opened existing semaphore[address=%p,name=%s]\n"
    : "Created new semaphore[address=%p,name=%s]\n";
    printf(fmt, (void*)ref_cnt_sem,
           ref_cnt_sem_name);
    queue_info->ref_count_sem = ref_cnt_sem;
    return QUEUE_INIT_SUCCESS;
}

InitStatus init_queue(QueueInfo* queue_info, TieType tie_type) {
    assert(queue_info);
    queue_info->type          = tie_type;
    queue_info->queue_id      = -1;
    queue_info->ref_count_sem = SEM_FAILED;
    int ret                   = init_queue_impl(queue_info);
    if (ret == QUEUE_INIT_FAILURE) {
        deinit_queue(queue_info);
    }

    return ret;
}

static bool deinit_semaphore(sem_t* sem) {
    false;
    bool should_delete = false;
    if (sem == SEM_FAILED) {
        return should_delete;
    }

    if (sem_trywait(sem) == -1) {
        if (errno != EAGAIN) {
            perror("sem_trywait");
        }
    } else {
        int val = 0;
        if (sem_getvalue(sem, &val) == -1) {
            perror("sem_getvalue");
        } else {
            assert(val >= 0);
            should_delete = val == 0;
        }

        if (sem_close(sem) == -1) {
            perror("sem_close");
        } else {
            printf("Closed semaphore[address=%p]\n", (void*)sem);
        }
    }

    return should_delete;
}

void deinit_queue(QueueInfo* queue_info) {
    assert(queue_info);
    mqd_t queue_id = queue_info->queue_id;
    if (queue_id == -1) {
        return;
    }

    bool should_delete = deinit_semaphore(queue_info->ref_count_sem);

    if (mq_close(queue_id) == -1) {
        perror("mq_close");
    } else {
        printf("Closed queue[queue_id=%d]\n", queue_id);
    }

    if (!should_delete) {
        return;
    }

    const char* queue_name       = get_queue_name(queue_info->type);
    const char* ref_cnt_sem_name = get_semaphore_name(queue_info->type);
    if (sem_unlink(ref_cnt_sem_name) == -1) {
        perror("sem_unlink");
    } else {
        printf("Unlinked semaphore[name=%s]\n", ref_cnt_sem_name);
    }
    if (mq_unlink(queue_name) == -1) {
        perror("mq_unlink");
    } else {
        printf("Unlinked queue[name=%s]\n", queue_name);
    }
}
