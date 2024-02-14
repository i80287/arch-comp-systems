#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#include <stdint.h>

static inline void rand_sleep(uint32_t max_sleep_seconds) {
    uint32_t sleep_time_sec = (uint32_t)rand() % (max_sleep_seconds + 1);
#ifdef _WIN32
    Sleep((DWORD)(sleep_time_sec * 1000));
#else
    sleep(sleep_time_sec);
#endif
}
