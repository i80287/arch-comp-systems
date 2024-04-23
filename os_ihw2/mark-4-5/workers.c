#include "workers.h"

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"

static void worker_shutdown_signal_handler(int signal) {
    assert(signal == SIGINT);
    printf("Worker[pid=%d] finished\n", getpid());
    exit(EXIT_SUCCESS);
}

static void setup_signal_handlers() {
    signal(SIGINT, worker_shutdown_signal_handler);
}

static bool check_pin_crookness(Pin produced_pin) {
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) + MIN_SLEEP_TIME;
    sleep(sleep_time);

    uint32_t x = (uint32_t)produced_pin.pin_id;
#if defined(__GNUC__)
    return __builtin_parity(x) & 1;
#else
    return x & 1;
#endif
}

static void sharpen_pin(Pin produced_pin) {
    (void)produced_pin;
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) + MIN_SLEEP_TIME;
    sleep(sleep_time);
    return;
}

static bool check_sharpened_pin_quality(Pin sharpened_pin) {
    uint32_t sleep_time =
        (uint32_t)rand() % (MAX_SLEEP_TIME - MIN_SLEEP_TIME) + MIN_SLEEP_TIME;
    sleep(sleep_time);
    return cos(sharpened_pin.pin_id) >= 0;
}

static bool wait_for_pin(SharedMemory* sh_mem, Pin* pin) {
    if (sem_wait(sh_mem->added_pins_sem) == -1) {
        perror("sem_wait[added pins sem]");
        return false;
    }

    if (sem_wait(sh_mem->buffer_access_sem) == -1) {
        perror("sem_wait[buffer access sem]");
        return false;
    }

    size_t read_index  = sh_mem->read_index;
    *pin               = sh_mem->buffer[read_index];
    sh_mem->read_index = (read_index + 1) % SH_MEM_BUFFER_SIZE;
    if (sem_post(sh_mem->free_pins_sem) == -1) {
        perror("sem_post[free pins sem]");
        return false;
    }

    if (sem_post(sh_mem->buffer_access_sem) == -1) {
        perror("sem_post[buffer access sem]");
        return false;
    }

    return true;
}

static bool send_pin(SharedMemory* sh_mem, Pin pin,
                     const char* message_format) {
    if (sem_wait(sh_mem->free_pins_sem) == -1) {
        perror("sem_wait[free pins sem]");
        return false;
    }

    if (sem_wait(sh_mem->buffer_access_sem) == -1) {
        perror("sem_wait[buffer access sem]");
        return false;
    }

    size_t write_index          = sh_mem->write_index;
    sh_mem->buffer[write_index] = pin;
    sh_mem->write_index         = (write_index + 1) % SH_MEM_BUFFER_SIZE;
    printf(message_format, getpid(), pin.pin_id);
    if (sem_post(sh_mem->added_pins_sem) == -1) {
        perror("sem_post[added pins sem]");
        return false;
    }

    if (sem_post(sh_mem->buffer_access_sem) == -1) {
        perror("sem_post[buffer access sem]");
        return false;
    }

    return true;
}

static bool send_produced_pin(SharedMemory* sh_mem, Pin pin) {
    return send_pin(
        sh_mem, pin,
        "+--------------------------------------------------------------\n"
        "| First worker[pid=%d] sent not crooked\n"
        "| pin[pin_id=%d] to the second workers.\n"
        "+--------------------------------------------------------------\n");
}

static bool send_sharpened_pin(SharedMemory* sh_mem, Pin pin) {
    return send_pin(
        sh_mem, pin,
        "+--------------------------------------------------------------\n"
        "| Second worker[pid=%d] sent sharpened\n"
        "| pin[pin_id=%d] to the third workers.\n"
        "+--------------------------------------------------------------\n");
}

static void run_first_worker(SharedMemory* first_to_second_sh_mem) {
    assert(first_to_second_sh_mem);
    const pid_t this_pid = getpid();
    while (true) {
        const Pin produced_pin = {.pin_id = rand()};
        printf(
            "+--------------------------------------------------------------\n"
            "| First worker[pid=%d] received pin[pin_id=%d]\n"
            "| and started checking it's crookness...\n"
            "+--------------------------------------------------------------\n",
            this_pid, produced_pin.pin_id);

        bool is_ok = check_pin_crookness(produced_pin);
        printf(
            "+--------------------------------------------------------------\n"
            "| First worker[pid=%d] decision:\n"
            "| pin[pin_id=%d] is %scrooked.\n"
            "+--------------------------------------------------------------\n",
            this_pid, produced_pin.pin_id, (is_ok ? "not " : ""));
        if (!is_ok) {
            continue;
        }

        if (!send_produced_pin(first_to_second_sh_mem, produced_pin)) {
            break;
        }
    }

    fprintf(stderr, "First worker[pid=%d] finished with error\n", this_pid);
    exit(EXIT_FAILURE);
}

static void run_second_worker(SharedMemory* first_to_second_sh_mem,
                              SharedMemory* second_to_third_sh_mem) {
    assert(first_to_second_sh_mem);
    assert(second_to_third_sh_mem);

    const pid_t this_pid = getpid();
    while (true) {
        Pin pin;
        if (!wait_for_pin(first_to_second_sh_mem, &pin)) {
            break;
        }

        printf(
            "+--------------------------------------------------------------\n"
            "| Second worker[pid=%d] received pin[pin_id=%d]\n"
            "| and started sharpening it...\n"
            "+--------------------------------------------------------------\n",
            this_pid, pin.pin_id);
        sharpen_pin(pin);
        printf(
            "+--------------------------------------------------------------\n"
            "| Second worker[pid=%d] sharpened pin[pin_id=%d].\n"
            "+--------------------------------------------------------------\n",
            this_pid, pin.pin_id);

        if (!send_sharpened_pin(second_to_third_sh_mem, pin)) {
            break;
        }
    }

    fprintf(stderr, "Second worker[pid=%d] finished with error\n", this_pid);
    exit(EXIT_FAILURE);
}

static void run_third_worker(SharedMemory* second_to_third_sh_mem) {
    assert(second_to_third_sh_mem);
    const pid_t this_pid = getpid();
    while (true) {
        Pin sharpened_pin;
        if (!wait_for_pin(second_to_third_sh_mem, &sharpened_pin)) {
            break;
        }

        printf(
            "+--------------------------------------------------------------\n"
            "| Third worker[pid=%d] received sharpened\n"
            "| pin[pin_id=%d] and started checking it's quality...\n"
            "+--------------------------------------------------------------\n",
            this_pid, sharpened_pin.pin_id);
        bool is_ok = check_sharpened_pin_quality(sharpened_pin);
        printf(
            "+--------------------------------------------------------------\n"
            "| Third worker[pid=%d] decision:\n"
            "| pin[pin_id=%d] is sharpened %s.\n"
            "+--------------------------------------------------------------\n",
            this_pid, sharpened_pin.pin_id, (is_ok ? "good enough" : "badly"));
    }

    fprintf(stderr, "First worker[pid=%d] finished with error\n", this_pid);
    exit(EXIT_FAILURE);
}

int start_first_workers(uint32_t first_workers,
                        SharedMemory* first_to_second_sh_mem) {
    const pid_t this_pid = getpid();
    for (uint32_t i = 0; i < first_workers; i++) {
        pid_t child_pid = fork();
        switch (child_pid) {
            case 0:
                setup_signal_handlers();
                run_first_worker(first_to_second_sh_mem);
                break;
            case -1:
                perror("fork[first worker creation error]");
                return EXIT_FAILURE;
            default:
                printf("Main process[pid=%d] started first worker[pid=%d]\n",
                       this_pid, child_pid);
                break;
        }
    }

    return EXIT_SUCCESS;
}

int start_second_workers(uint32_t second_workers,
                         SharedMemory* first_to_second_sh_mem,
                         SharedMemory* second_to_third_sh_mem) {
    const pid_t this_pid = getpid();
    for (uint32_t i = 0; i < second_workers; i++) {
        pid_t child_pid = fork();
        switch (child_pid) {
            case 0:
                setup_signal_handlers();
                run_second_worker(first_to_second_sh_mem,
                                  second_to_third_sh_mem);
                break;
            case -1:
                perror("fork[second worker creation error]");
                return EXIT_FAILURE;
            default:
                printf("Main process[pid=%d] started second worker[pid=%d]\n",
                       this_pid, child_pid);
                break;
        }
    }

    return EXIT_SUCCESS;
}

int start_third_workers(uint32_t third_workers,
                        SharedMemory* second_to_third_sh_mem) {
    const pid_t this_pid = getpid();
    for (uint32_t i = 0; i < third_workers; i++) {
        pid_t child_pid = fork();
        switch (child_pid) {
            case 0:
                setup_signal_handlers();
                run_third_worker(second_to_third_sh_mem);
                break;
            case -1:
                perror("fork[third worker creation error]");
                return EXIT_FAILURE;
            default:
                printf("Main process[pid=%d] started third worker[pid=%d]\n",
                       this_pid, child_pid);
                break;
        }
    }

    return EXIT_SUCCESS;
}
