#include "resources.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef enum ResourceTieType {
    FIRST_TO_SECOND_WORKER,
    SECOND_TO_THIRD_WORKER,
} ResourceTieType;

static ResourceInitStatus init_shared_data(struct SharedData* sh_data,
                                           ResourceTieType resourse_tie_type) {
    assert(sh_data);
    const char* sh_mem_name;
    const char* access_sem_name;
    const char* added_pins_sem_name;
    const char* free_pins_sem_name;
    switch (resourse_tie_type) {
        case FIRST_TO_SECOND_WORKER:
            sh_mem_name         = FIRST_TO_SECOND_WORKER_SHARED_MEMORY;
            access_sem_name     = FIRST_TO_SECOND_WORKER_ACCESS_SEM_NAME;
            added_pins_sem_name = FIRST_TO_SECOND_WORKER_ADD_SEM_NAME;
            free_pins_sem_name  = FIRST_TO_SECOND_WORKER_FREE_SEM_NAME;
            break;
        case SECOND_TO_THIRD_WORKER:
            sh_mem_name         = SECOND_TO_THIRD_WORKER_SHARED_MEMORY;
            access_sem_name     = SECOND_TO_THIRD_WORKER_ACCESS_SEM_NAME;
            added_pins_sem_name = SECOND_TO_THIRD_WORKER_ADD_SEM_NAME;
            free_pins_sem_name  = SECOND_TO_THIRD_WORKER_FREE_SEM_NAME;
            break;
        default:
            return RESOURCE_INIT_FAILURE;
    }

    sh_data->fd = shm_open(sh_mem_name, SHARED_MEMORY_OPEN_FLAGS,
                           SHARED_MEMORY_OPEN_PERMS_MODE);
    if (sh_data->fd == -1) {
        perror("shm_open");
        return RESOURCE_INIT_FAILURE;
    }
    if (ftruncate(sh_data->fd, SHARED_MEMORY_OPEN_PERMS_MODE) == -1) {
        perror("ftruncate");
        return RESOURCE_INIT_FAILURE;
    }

    SharedMemory* sh_mem = (SharedMemory*)mmap(0, SHARED_MEMORY_SIZE, MMAP_PROT,
                                               MMAP_FLAGS, sh_data->fd, 0);
    sh_data->sh_mem      = sh_mem;
    if (sh_mem == MAP_FAILED) {
        perror("mmap");
        return RESOURCE_INIT_FAILURE;
    }

    memset(sh_mem, 0, sizeof(*sh_mem));
    sh_mem->buffer_access_sem = SEM_FAILED;
    sh_mem->added_pins_sem    = SEM_FAILED;
    sh_mem->free_pins_sem     = SEM_FAILED;

    const unsigned access_sem_value = 1;
    sh_mem->buffer_access_sem =
        sem_open(access_sem_name, SEMAPHORE_OPEN_FLAGS,
                 SEMAPHORE_OPEN_PERMS_MODE, access_sem_value);
    if (sh_mem->buffer_access_sem == SEM_FAILED) {
        perror("sem_open[buffer access semaphore]");
        return RESOURCE_INIT_FAILURE;
    }

    const unsigned added_pins_sem_value = 0;
    sh_mem->added_pins_sem =
        sem_open(added_pins_sem_name, SEMAPHORE_OPEN_FLAGS,
                 SEMAPHORE_OPEN_PERMS_MODE, added_pins_sem_value);
    if (sh_mem->added_pins_sem == SEM_FAILED) {
        perror("sem_open[added pins semaphore]");
        return RESOURCE_INIT_FAILURE;
    }

    const unsigned free_pins_sem_value = SH_MEM_BUFFER_SIZE;
    sh_mem->free_pins_sem =
        sem_open(free_pins_sem_name, SEMAPHORE_OPEN_FLAGS,
                 SEMAPHORE_OPEN_PERMS_MODE, free_pins_sem_value);
    if (sh_mem->free_pins_sem == SEM_FAILED) {
        perror("sem_open[free pins semaphore]");
        return RESOURCE_INIT_FAILURE;
    }

    return RESOURCE_INIT_SUCCESS;
}

static ResourceInitStatus init_resources_impl(Resources* res) {
    if (init_shared_data(&res->first_to_second_sd, FIRST_TO_SECOND_WORKER) !=
        RESOURCE_INIT_SUCCESS) {
        return RESOURCE_INIT_FAILURE;
    }
    return init_shared_data(&res->second_to_third_sd, SECOND_TO_THIRD_WORKER);
}

ResourceInitStatus init_resources(Resources* res) {
    assert(res);
    res->first_to_second_sd.fd     = -1;
    res->second_to_third_sd.fd     = -1;
    res->first_to_second_sd.sh_mem = MAP_FAILED;
    res->second_to_third_sd.sh_mem = MAP_FAILED;

    int ret = init_resources_impl(res);
    if (ret == RESOURCE_INIT_FAILURE) {
        deinit_resources(res);
    }

    return ret;
}

static void deinit_semaphore(sem_t* sem, const char* sem_name) {
    if (sem != SEM_FAILED) {
        if (sem_close(sem) == -1) {
            perror("sem_close");
        }
        if (sem_unlink(sem_name) == -1) {
            perror("sem_unlink");
        }
    }
}

static void deinit_shared_data(struct SharedData* sh_data,
                               ResourceTieType resourse_tie_type) {
    assert(sh_data);
    const char* sh_mem_name;
    const char* access_sem_name;
    const char* added_pins_sem_name;
    const char* free_pins_sem_name;
    switch (resourse_tie_type) {
        case FIRST_TO_SECOND_WORKER:
            sh_mem_name         = FIRST_TO_SECOND_WORKER_SHARED_MEMORY;
            access_sem_name     = FIRST_TO_SECOND_WORKER_ACCESS_SEM_NAME;
            added_pins_sem_name = FIRST_TO_SECOND_WORKER_ADD_SEM_NAME;
            free_pins_sem_name  = FIRST_TO_SECOND_WORKER_FREE_SEM_NAME;
            break;
        case SECOND_TO_THIRD_WORKER:
            sh_mem_name         = SECOND_TO_THIRD_WORKER_SHARED_MEMORY;
            access_sem_name     = SECOND_TO_THIRD_WORKER_ACCESS_SEM_NAME;
            added_pins_sem_name = SECOND_TO_THIRD_WORKER_ADD_SEM_NAME;
            free_pins_sem_name  = SECOND_TO_THIRD_WORKER_FREE_SEM_NAME;
            break;
        default:
            return;
    }

    SharedMemory* sh_mem = sh_data->sh_mem;
    if (sh_mem != MAP_FAILED) {
        deinit_semaphore(sh_mem->free_pins_sem, free_pins_sem_name);
        deinit_semaphore(sh_mem->added_pins_sem, added_pins_sem_name);
        deinit_semaphore(sh_mem->buffer_access_sem, access_sem_name);
        if (munmap(sh_mem, SHARED_MEMORY_SIZE) == -1) {
            perror("munmap");
        }
    }

    int sh_mem_fd = sh_data->fd;
    if (sh_mem_fd != -1) {
        if (close(sh_mem_fd) == -1) {
            perror("close");
        }
        if (shm_unlink(sh_mem_name) == -1) {
            perror("shm_unlink");
        }
    }
}

void deinit_resources(Resources* res) {
    assert(res);
    deinit_shared_data(&res->second_to_third_sd, SECOND_TO_THIRD_WORKER);
    deinit_shared_data(&res->first_to_second_sd, FIRST_TO_SECOND_WORKER);
}
