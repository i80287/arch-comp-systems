#include "resources.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static InitStatus init_semaphore(key_t sem_key, uint32_t sem_initial_value,
                                 int* sem_id, int new_sems_id[3],
                                 size_t* new_sems_size) {
    *sem_id           = semget(sem_key, 1, NEW_SEMAPHORE_OPEN_FLAGS);
    const bool failed = *sem_id == -1 && errno != EEXIST;
    if (failed) {
        perror("semget");
        return SHARED_MEMORY_INIT_FAILURE;
    }

    const bool sem_already_exists = *sem_id == -1;
    if (sem_already_exists) {
        *sem_id = semget(sem_key, 1, EXISTING_SEMAPHORE_OPEN_FLAGS);
    }

    if (*sem_id == -1) {
        perror("semget");
        return SHARED_MEMORY_INIT_FAILURE;
    }

    if (!sem_already_exists) {
        printf("Created new semaphore[sem_id=%d]\n", *sem_id);
        new_sems_id[*new_sems_size++] = *sem_id;
        if (semctl(*sem_id, 0, SETVAL, sem_initial_value) == -1) {
            return SHARED_MEMORY_INIT_FAILURE;
        }
    } else {
        printf("Opened existing semaphore[sem_id=%d]\n", *sem_id);
    }

    return SHARED_MEMORY_INIT_SUCCESS;
}

static InitStatus init_shared_memory(SharedMemory* sh_mem, int access_sem_key,
                                     int added_pins_sem_key,
                                     int free_pins_sem_key, int new_sems_id[3],
                                     size_t* new_sems_size) {
    memset(sh_mem, 0, sizeof(*sh_mem));
    sh_mem->buffer_access_sem_id = -1;
    sh_mem->added_pins_sem_id    = -1;
    sh_mem->free_pins_sem_id     = -1;
    sh_mem->sh_mem_ref_counter   = 1;

    int buffer_access_sem_id;
    int added_pins_sem_id;
    int free_pins_sem_id;
    InitStatus res;

    res = init_semaphore(access_sem_key, ACCESS_SEM_DEFAULT_VALUE,
                         &buffer_access_sem_id, new_sems_id, new_sems_size);
    if (res == SHARED_MEMORY_INIT_FAILURE) {
        return res;
    }

    res = init_semaphore(added_pins_sem_key, ADD_SEM_DEFAULT_VALUE,
                         &added_pins_sem_id, new_sems_id, new_sems_size);
    if (res == SHARED_MEMORY_INIT_FAILURE) {
        return res;
    }

    res = init_semaphore(free_pins_sem_key, FREE_SEM_DEFAULT_VALUE,
                         &free_pins_sem_id, new_sems_id, new_sems_size);
    if (res == SHARED_MEMORY_INIT_FAILURE) {
        return res;
    }

    sh_mem->buffer_access_sem_id = buffer_access_sem_id;
    sh_mem->added_pins_sem_id    = added_pins_sem_id;
    sh_mem->free_pins_sem_id     = free_pins_sem_id;
    return SHARED_MEMORY_INIT_SUCCESS;
}

static InitStatus init_resources_impl(SharedMemoryInfo* sh_mem_info) {
    const TieType tie_type = sh_mem_info->type;
    assert(tie_type <= SECOND_TO_THIRD_WORKER);
    const int shared_mem_keys[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_SHARED_MEMORY_KEY,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_SHARED_MEMORY_KEY,
    };
    const int access_sem_keys[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_ACCESS_SEM_KEY,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_ACCESS_SEM_KEY,
    };
    const int added_pins_sem_keys[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_ADD_SEM_KEY,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_ADD_SEM_KEY,
    };
    const int free_pins_sem_keys[] = {
        [FIRST_TO_SECOND_WORKER] = FIRST_TO_SECOND_WORKER_FREE_SEM_KEY,
        [SECOND_TO_THIRD_WORKER] = SECOND_TO_THIRD_WORKER_FREE_SEM_KEY,
    };
    const int shared_memory_key  = shared_mem_keys[tie_type];
    const int access_sem_key     = access_sem_keys[tie_type];
    const int added_pins_sem_key = added_pins_sem_keys[tie_type];
    const int free_pins_sem_key  = free_pins_sem_keys[tie_type];

    int sh_mem_id =
        shmget(shared_memory_key, SHARED_MEMORY_SIZE, NEW_SHARED_MEMORY_FLAGS);
    const bool failed = sh_mem_id == -1 && errno != EEXIST;
    if (failed) {
        perror("shmget");
        return SHARED_MEMORY_INIT_FAILURE;
    }
    const bool memory_already_exists = sh_mem_id == -1;
    if (memory_already_exists) {
        sh_mem_id = shmget(shared_memory_key, SHARED_MEMORY_SIZE,
                           EXISTING_SHARED_MEMORY_FLAGS);
    }
    if (sh_mem_id == -1) {
        perror("shmget");
        return SHARED_MEMORY_INIT_FAILURE;
    }

    const char* fmt = memory_already_exists
                          ? "Got existing shared memory[sh_mem_id=%d]\n"
                          : "Created new shared memory[sh_mem_id=%d]\n";
    printf(fmt, sh_mem_id);

    sh_mem_info->shared_mem_id = sh_mem_id;
    SharedMemory* sh_mem       = (SharedMemory*)shmat(sh_mem_id, 0, 0);
    if (sh_mem_info == NULL || sh_mem_info == (void*)-1) {
        perror("shmat");
        return SHARED_MEMORY_INIT_FAILURE;
    }
    sh_mem_info->sh_mem = sh_mem;
    printf("Attached shared memory[address=%p]\n", (void*)sh_mem);

    if (memory_already_exists) {
        ++sh_mem->sh_mem_ref_counter;
    } else {
        int new_sems_id[3]   = {0};
        size_t new_sems_size = 0;
        int ret =
            init_shared_memory(sh_mem, access_sem_key, added_pins_sem_key,
                               free_pins_sem_key, new_sems_id, &new_sems_size);
        if (ret == SHARED_MEMORY_INIT_FAILURE) {
            assert(new_sems_size <= 3);
            for (size_t i = 0; i < new_sems_size; i++) {
                if (semctl(new_sems_id[i], 0, IPC_RMID) == -1) {
                    perror("semctl");
                }
            }
            return ret;
        }
    }

    return SHARED_MEMORY_INIT_SUCCESS;
}

InitStatus init_shared_memory_info(SharedMemoryInfo* res, TieType tie_type) {
    assert(res);
    res->type          = tie_type;
    res->shared_mem_id = -1;
    res->sh_mem        = (void*)-1;
    int ret            = init_resources_impl(res);
    if (ret == SHARED_MEMORY_INIT_FAILURE) {
        deinit_shared_memory_info(res);
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

static void delete_semaphores(SharedMemory* sh_mem) {
    delete_semaphore(sh_mem->free_pins_sem_id);
    delete_semaphore(sh_mem->added_pins_sem_id);
    delete_semaphore(sh_mem->buffer_access_sem_id);
}

void deinit_shared_memory_info(SharedMemoryInfo* res) {
    assert(res);
    int sh_mem_id = res->shared_mem_id;
    if (sh_mem_id == -1) {
        return;
    }

    SharedMemory* const sh_mem = res->sh_mem;
    if (sh_mem == NULL || sh_mem == (void*)-1) {
        return;
    }

    int ref_cnt = sh_mem->sh_mem_ref_counter;
    assert(ref_cnt >= 0);
    const bool delete_sh_mem = ref_cnt > 0 && --sh_mem->sh_mem_ref_counter == 0;
    if (delete_sh_mem) {
        delete_semaphores(sh_mem);
    }

    if (shmdt(sh_mem) == -1) {
        perror("shmdt");
    } else {
        printf("Detached shared memory[address=%p]\n", (void*)sh_mem);
    }

    if (!delete_sh_mem) {
        return;
    }
    if (shmctl(sh_mem_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    } else {
        printf("Deleted shared memory[sh_mem_id=%d]\n", sh_mem_id);
    }
}
