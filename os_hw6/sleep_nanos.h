#pragma once

// Defined to define _POSIX_C_SOURCE 200809L to enable nanosleep
#define _XOPEN_SOURCE 700
#include <time.h>

static int sleep_nanos(long nanoseconds) {
    const struct timespec sleep_time = {
        .tv_sec  = 0,
        .tv_nsec = nanoseconds,
    };
    return nanosleep(&sleep_time, NULL);
}
