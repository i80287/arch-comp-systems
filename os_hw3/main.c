#include <assert.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"

/// @brief Function to work out Fibonacci number F_n in O(log(n)).
///        Here we suppose that F_0 = 1, F_1 = 1, F_2 = 2 and so on.
/// @param n
/// @param fib_num n-th Fibonacci number will be stored in this argument
/// @return true if overflow occured during calculations and false otherwise.
bool calculate_fibonacci_num(uint32_t n, uint64_t* fib_num) {
    assert(fib_num);

    // Basic algorithm to calculate fibonacci number in O(log(n))
    // using matrix multiplication.
    const bool overflow_occured = n >= 93;

    uint64_t pow_matrix[2][2] = {
        {0, 1},
        {1, 1},
    };
    uint64_t fibmatrix[2][2] = {
        {1, 0},
        {0, 1},
    };

    while (true) {
        if (n % 2 != 0) {
            const uint64_t tmp[2][2] = {
                {fibmatrix[0][0] * pow_matrix[0][0] + fibmatrix[0][1] * pow_matrix[1][0],
                 fibmatrix[0][0] * pow_matrix[0][1] + fibmatrix[0][1] * pow_matrix[1][1]},
                {fibmatrix[1][0] * pow_matrix[0][0] + fibmatrix[1][1] * pow_matrix[1][0],
                 fibmatrix[1][0] * pow_matrix[0][1] + fibmatrix[1][1] * pow_matrix[1][1]},
            };
            fibmatrix[0][0] = tmp[0][0];
            fibmatrix[0][1] = tmp[0][1];
            fibmatrix[1][0] = tmp[1][0];
            fibmatrix[1][1] = tmp[1][1];
        }

        n /= 2;
        if (n == 0) {
            break;
        }

        const uint64_t tmp[2][2] = {
            {pow_matrix[0][0] * pow_matrix[0][0] + pow_matrix[0][1] * pow_matrix[1][0],
             pow_matrix[0][0] * pow_matrix[0][1] + pow_matrix[0][1] * pow_matrix[1][1]},
            {pow_matrix[1][0] * pow_matrix[0][0] + pow_matrix[1][1] * pow_matrix[1][0],
             pow_matrix[1][0] * pow_matrix[0][1] + pow_matrix[1][1] * pow_matrix[1][1]},
        };
        pow_matrix[0][0] = tmp[0][0];
        pow_matrix[0][1] = tmp[0][1];
        pow_matrix[1][0] = tmp[1][0];
        pow_matrix[1][1] = tmp[1][1];
    }

    *fib_num = fibmatrix[0][0] + fibmatrix[1][0];
    return overflow_occured;
}

/// @brief Function to calculate n! in O(n)
/// @param n
/// @param factorial n! will be stored in this argument
/// @return true if overflow occured during calculations and false otherwise.
bool calculate_factorial(uint32_t n, uint64_t* factorial) {
    // Btw if n >= 66, factorial will be 0, because 66! = 2^64 * s, s is odd
    assert(factorial);
    bool overflow_occured  = false;
    unsigned long long ans = 1;
    for (uint64_t i = 2; i <= n; i++) {
        // __builtin_umulll_overflow is a gcc's builtin function
        // that returns true iff overflow occured.
        // For more info see:
        // https://gcc.gnu.org/onlinedocs/gcc/Integer-Overflow-Builtins.html
        overflow_occured |= __builtin_umulll_overflow(ans, i, &ans);
    }
    *factorial = ans;
    return overflow_occured;
}

pid_t create_fibonacci_process(uint32_t fib_num_index) {
    pid_t child_proc_id = fork();
    switch (child_proc_id) {
        case -1:
            perror("fork failed during creation of Fibonacci purpose process\n");
            exit(EXIT_FAILURE);
        default:
            return child_proc_id;
        case 0: {
            printf(
                "Started child process with id %d of the parent process with id %d.\n"
                "  This process will calculate %u-th Fibonacci number\n\n",
                getpid(), getppid(), fib_num_index);

            uint64_t fib_num      = 0;
            bool overflow_occured = calculate_fibonacci_num(fib_num_index, &fib_num);
            if (overflow_occured) {
                printf("An overflow occured while calculating Fibonacci number\n");
            }
            printf("Calculated %u-th Fibonacci number = %" PRIu64 "\n\n", fib_num_index,
                   fib_num);
            exit(EXIT_SUCCESS);
        }
    }
}

pid_t create_factorial_process(uint32_t fact_index) {
    pid_t child_proc_id = fork();
    switch (child_proc_id) {
        case -1:
            perror("fork failed during creation of factorial purpose process\n");
            exit(EXIT_FAILURE);
        default:
            return child_proc_id;
        case 0: {
            printf(
                "Started child process with id %d of the parent process with id %d.\n"
                "  This process will calculate factorial of %u\n\n",
                getpid(), getppid(), fact_index);

            uint64_t factorial    = 0;
            bool overflow_occured = calculate_factorial(fact_index, &factorial);
            if (overflow_occured) {
                printf("An overflow occured while calculating factorial\n");
            }
            printf("Calculated %" PRIu32 "! = %" PRIu64 "\n\n", fact_index, factorial);
            exit(EXIT_SUCCESS);
        }
    }
}

pid_t create_list_dir_process() {
    pid_t child_proc_id = fork();
    switch (child_proc_id) {
        case -1:
            perror("fork failed during creation of list dir purpose process\n");
            exit(EXIT_FAILURE);
        default:
            return child_proc_id;
        case 0: {
            printf(
                "Started child process with id %d of the parent process with id %d.\n"
                "  This process will list information about the current directory\n\n",
                getpid(), getppid());

            char buffer[PATH_MAX];
            if (getcwd(buffer, PATH_MAX)) {
                printf("Current working directory: '%s'\n", buffer);
            }

            printf("Files in the current working directory:\n");
            int res = system("ls -la");
            if (res == -1) {
                printf("Could not print this information\n");
            }
            exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, const char* const argv[]) {
    parse_result_t result = parse_arguments(argc, argv);
    if (result.status != PARSE_OK) {
        handle_parsing_error(result.status);
        return EXIT_FAILURE;
    }

    // Create new process that will calculate needed factorial and print the results
    pid_t fact_proc_id = create_factorial_process(result.fact_index);
    printf("Created child process with id %d to calculate factorial of %u\n\n",
           fact_proc_id, result.fact_index);

    // Create new process the will calculate needed fibonacci number and print the results
    pid_t fib_proc_id = create_fibonacci_process(result.fib_num_index);
    printf("Created child process with id %d to calculate %u-th Fibonacci number\n\n",
           fact_proc_id, result.fib_num_index);

    // Wait for the created child processes
    waitpid(fib_proc_id, NULL, 0);
    waitpid(fact_proc_id, NULL, 0);

    pid_t dir_list_proc = create_list_dir_process();
    printf(
        "Created child process with id %d to list information about current "
        "directory\n\n",
        dir_list_proc);

    waitpid(dir_list_proc, NULL, 0);
    return EXIT_SUCCESS;
}
