#pragma once

// Defined to define _POSIX_C_SOURCE 200809L to enable nanosleep
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <time.h>

static const long DEFAULT_SLEEP_SPINLOCK_INTERVAL = 20000;

static int sleep_nanos(long nanoseconds) {
    const struct timespec sleep_time = {
        .tv_sec  = 0,
        .tv_nsec = nanoseconds,
    };
    return nanosleep(&sleep_time, NULL);
}
