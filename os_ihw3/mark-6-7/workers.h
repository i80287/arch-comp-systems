#ifndef WORKERS_H
#define WORKERS_H

#include "memory_config.h"

int start_first_workers(uint32_t first_workers, SharedMemory * first_to_second_sh_mem);
int start_second_workers(uint32_t second_workers, SharedMemory* first_to_second_sh_mem, SharedMemory* second_to_third_sh_mem);
int start_third_workers(uint32_t third_workers, SharedMemory* second_to_third_sh_mem);

#endif
