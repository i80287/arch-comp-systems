#pragma once

#include <stdbool.h>  // for bool
#include <stddef.h>   // for size_t
#include <string.h>   // for memcmp

#include "pin.h"  // for Pin

static const char SHUTDOWN_MESSAGE[] = "0x00000";
enum {
    SHUTDOWN_MESSAGE_SIZE = sizeof(SHUTDOWN_MESSAGE),
};

static inline bool is_shutdown_message(const char* bytes, size_t size) {
    return size == SHUTDOWN_MESSAGE_SIZE &&
           memcmp(bytes, SHUTDOWN_MESSAGE, SHUTDOWN_MESSAGE_SIZE) == 0;
}

enum {
    NET_BUFFER_SIZE = sizeof(Pin) > SHUTDOWN_MESSAGE_SIZE ? sizeof(Pin) : SHUTDOWN_MESSAGE_SIZE
};
