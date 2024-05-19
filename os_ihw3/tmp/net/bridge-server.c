#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parser.h"
#include "server.h"

static int run_worker_handler(const Server server,
                              struct sockaddr_in worker_addr,
                              WorkerType type, size_t worker_array_index) {
    /* worker_addr is implicitly passed by value */
    
}

static int start_runtime_loop(Server server) {
    int ret = EXIT_SUCCESS;
    for (bool is_running = true; is_running;) {
        struct sockaddr_in worker_addr;
        WorkerType type;
        size_t insert_index;
        if (!server_accept_worker(server, &worker_addr, &type,
                                  &insert_index)) {
            continue;
        }

        pid_t child_pid = fork();
        switch (child_pid) {
            case -1: {
                perror("fork");
                ret        = EXIT_FAILURE;
                is_running = false;
            } break;
            case 0: {
                int exit_code = run_worker_handler(server, worker_addr,
                                                   type, insert_index);
                printf(
                    "Worker handler child process[pid=%d] ended with code "
                    "%d\n",
                    getpid(), exit_code);
                exit(exit_code);
            } break;
            default: {
                printf(
                    "Started new process[pid=%d] for handling new "
                    "worker[port=%u, type=%d]\n",
                    child_pid, worker_addr.sin_port, type);
            } break;
        }
    }
}

static int run_server(uint16_t server_port) {
    Server server;
    if (!init_server(server, server_port)) {
        return EXIT_FAILURE;
    }

    int ret = start_runtime_loop(server);
    wait_for_children();
    deinit_server(server);
    return ret;
}

int main(int argc, const char *argv[]) {
    ParseResultServer res = parse_args_server(argc, argv);
    if (res.status != PARSE_SUCCESS) {
        print_invalid_args_error_server(res.status, argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(res.port);
}
