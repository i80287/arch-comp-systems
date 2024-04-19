#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/wait.h>

enum { MAX_MESSAGE_SIZE = 128 };
enum { SLEEP_SECONDS = 2U };

static volatile bool is_running  = true;
static volatile pid_t parent_pid = -1;
static volatile pid_t child_pid  = -1;

static void signal_handler(int signal) {
    assert(parent_pid != -1);
    assert(child_pid != -1);
    assert(signal == SIGINT);

    const pid_t this_process_id   = getpid();
    const bool is_in_main_process = this_process_id == parent_pid;
    is_running                    = false;
    const char* message           = is_in_main_process
                                        ? "Parent[pid=%d] received shutdown signal\n"
                                        : "Child[pid=%d] received shutdown signal\n";
    printf(message, this_process_id);
}

static int run_child_process_impl(int sem_id, int fd[2]) {
    char send_message_buffer[MAX_MESSAGE_SIZE] = {0};
    char read_message_buffer[MAX_MESSAGE_SIZE] = {0};
    uint32_t message_counter                   = 0;

    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = 0,
        .sem_flg = 0,
    };

    while (is_running) {
        // Subtract 1 from semaphore
        buffer.sem_op = -1;
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        // Read from pipe
        const ssize_t bytes_read =
            read(fd[0], read_message_buffer, sizeof(read_message_buffer));
        if (bytes_read < 0) {
            perror("read");
            return EXIT_FAILURE;
        }

        // Check for empty content
        if (bytes_read != 1) {
            printf("\n> Message from parent:\n'%s'\n> End of message.\n",
                   read_message_buffer);
        }

        // Write to pipe
        const int bytes_written =
            sprintf(send_message_buffer,
                    "Some message %u from child[pid=%d] to parent[pid=%d]",
                    ++message_counter, child_pid, parent_pid);
        if (bytes_written < 0) {
            perror("sprintf");
            return EXIT_FAILURE;
        }
        const size_t message_size_with_null_term =
            (uint32_t)bytes_written + 1;
        assert(message_size_with_null_term <= MAX_MESSAGE_SIZE);
        if (write(fd[1], send_message_buffer,
                  message_size_with_null_term) < 0) {
            perror("write");
            return EXIT_FAILURE;
        }

        // Add 1 to semaphore [child->parent]
        buffer.sem_op = 1;
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        sleep(SLEEP_SECONDS);
    }

    return EXIT_SUCCESS;
}

static int run_child_process(int sem_id, int fd[2]) {
    const int res = run_child_process_impl(sem_id, fd);
    close(fd[0]);
    close(fd[1]);
    return res;
}

static int run_main_process_impl(int sem_id, int fd[2]) {
    char send_message_buffer[MAX_MESSAGE_SIZE] = {0};
    char read_message_buffer[MAX_MESSAGE_SIZE] = {0};
    uint32_t message_counter                   = 0;

    struct sembuf buffer = {
        .sem_num = 0,
        .sem_op  = 0,
        .sem_flg = 0,
    };

    while (is_running) {
        // Subtract 1 from semaphore
        buffer.sem_op = -1;
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        // Read from pipe
        const ssize_t bytes_read =
            read(fd[0], read_message_buffer, sizeof(read_message_buffer));
        if (bytes_read < 0) {
            perror("read");
            return EXIT_FAILURE;
        }
        // Check for empty content
        if (bytes_read != 1) {
            printf("\n> Message from child:\n'%s'\n> End of message.\n",
                   read_message_buffer);
        }

        // Write to pipe
        const int bytes_written =
            sprintf(send_message_buffer,
                    "Some message %u from parent[pid=%d] to child[pid=%d]",
                    ++message_counter, parent_pid, child_pid);
        if (bytes_written < 0) {
            perror("sprintf");
            return EXIT_FAILURE;
        }
        const size_t message_size_with_null_term =
            (uint32_t)bytes_written + 1;
        assert(message_size_with_null_term <= MAX_MESSAGE_SIZE);
        if (write(fd[1], send_message_buffer,
                  message_size_with_null_term) < 0) {
            perror("write");
            return EXIT_FAILURE;
        }

        // Add 1 to semaphore [child->parent]
        buffer.sem_op = 1;
        if (semop(sem_id, &buffer, 1) == -1) {
            perror("semop");
            return EXIT_FAILURE;
        }

        sleep(SLEEP_SECONDS);
    }

    return EXIT_SUCCESS;
}

static int run_main_process(int sem_id, int fd[2]) {
    int res = run_main_process_impl(sem_id, fd);
    close(fd[0]);
    close(fd[1]);

    // Wait for child
    if (wait(0) == (pid_t)-1) {
        perror("wait");
        res = EXIT_FAILURE;
    }
    // Free semaphore
    if (semctl(sem_id, 0, IPC_RMID, 0) < 0) {
        perror("semctl");
        res = EXIT_FAILURE;
    }

    return res;
}

static void setup_signal_handlers() {
    signal(SIGINT, signal_handler);
}

int main() {
    setup_signal_handlers();

    // Create pipe
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    // Initialize buffer with empty content
    if (write(fd[1], "\0", 1) < 0) {
        perror("write");
        return EXIT_FAILURE;
    }

    const char pathname[]          = "main.c";
    const key_t key                = ftok(pathname, 0);
    const int number_of_semaphores = 1;
    // Create or get semaphore
    int sem_id = semget(key, number_of_semaphores, 0666 | IPC_CREAT);
    if (sem_id < 0) {
        perror("semget");
        return EXIT_FAILURE;
    }
    // Set semaphore to 1
    semctl(sem_id, 0, SETVAL, 1);

    // Create child process
    parent_pid = getpid();
    child_pid  = fork();
    switch (child_pid) {
        case 0:
            child_pid = getpid();
            return run_child_process(sem_id, fd);
        case -1:
            perror("fork");
            return EXIT_FAILURE;
        default:
            return run_main_process(sem_id, fd);
    }
}
