#ifndef RESOURCES_H
#define RESOURCES_H

#include "memory_config.h"

struct SharedData {
    int32_t fd;
    SharedMemory* sh_mem;
};

typedef struct Resources {
    struct SharedData first_to_second_sd;
    struct SharedData second_to_third_sd;
} Resources;

typedef enum ResourceInitStatus {
    RESOURCE_INIT_SUCCESS,
    RESOURCE_INIT_FAILURE
} ResourceInitStatus;

void deinit_resources(Resources* res);

ResourceInitStatus init_resources(Resources* res);

#endif
