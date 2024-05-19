#include "server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static bool setup_server(int server_sock_fd,
                         struct sockaddr_in* server_sock_addr,
                         uint16_t server_port) {
    server_sock_addr->sin_family      = AF_INET;
    server_sock_addr->sin_port        = htons(server_port);
    server_sock_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    bool bind_failed =
        bind(server_sock_fd, (const struct sockaddr*)server_sock_addr,
             sizeof(*server_sock_addr)) == -1;
    if (bind_failed) {
        perror("bind");
        return false;
    }

    bool listen_failed =
        listen(server_sock_fd, MAX_CONNECTIONS_PER_SERVER) == -1;
    if (listen_failed) {
        perror("listen");
        return false;
    }

    return true;
}

bool init_server(Server server, uint16_t server_port) {
    memset(server, 0, sizeof(*server));
    server->sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->sock_fd == -1) {
        perror("socket");
        return false;
    }

    if (!setup_server(server->sock_fd, &server->sock_addr, server_port)) {
        close(server->sock_fd);
        return false;
    }

    memset(server->first_workers_fds, -1,
           sizeof(int) * MAX_NUMBER_OF_FIRST_WORKERS);
    memset(server->second_workers_fds, -1,
           sizeof(int) * MAX_NUMBER_OF_SECOND_WORKERS);
    memset(server->third_workers_fds, -1,
           sizeof(int) * MAX_NUMBER_OF_THIRD_WORKERS);
    return true;
}

static void close_workers_fds(int fds[], size_t array_max_size) {
    for (size_t i = 0; i < array_max_size; i++) {
        if (fds[i] != -1) {
            close(fds[i]);
        }
    }
}

void deinit_server(Server server) {
    close_workers_fds(server->third_workers_fds,
                      MAX_NUMBER_OF_THIRD_WORKERS);
    close_workers_fds(server->second_workers_fds,
                      MAX_NUMBER_OF_SECOND_WORKERS);
    close_workers_fds(server->first_workers_fds,
                      MAX_NUMBER_OF_FIRST_WORKERS);
    close(server->sock_fd);
}

static bool receive_worker_type(int worker_sock_fd,
                                const struct sockaddr_in* worker_addr,
                                WorkerType* type) {
    union {
        char bytes[sizeof(*type)];
        WorkerType type;
    } buffer = {0};

    bool received_type;
    int tries = 4;
    do {
        received_type =
            recv(worker_sock_fd, buffer.bytes, sizeof(buffer.bytes),
                 MSG_DONTWAIT) == sizeof(buffer.bytes);
    } while (!received_type && --tries > 0);

    if (!received_type) {
        return false;
    }

    *type = buffer.type;
    return true;
}

static size_t insert_new_worker_into_arrays(
    struct sockaddr_in workers_addrs[], int workers_fds[],
    size_t* array_size, size_t max_array_size,
    const struct sockaddr_in* worker_addr, int worker_sock_fd) {
    assert(*array_size <= max_array_size);
    for (size_t i = 0; i < max_array_size; i++) {
        int invalid_fd = -1;
        // if workers_fds[i] == -1, write worker_sock_fd to workers_fds[i] and return true
        if (!atomic_compare_exchange_strong(&workers_fds[i], &invalid_fd, worker_sock_fd)) {
            continue;
        }
        assert(*array_size < max_array_size);
        workers_fds[i]   = worker_sock_fd;
        workers_addrs[i] = *worker_addr;
        (*array_size)++;
        return i;
    }

    return (size_t)-1;
}

static bool handle_new_worker(Server server,
                              const struct sockaddr_in* worker_addr,
                              socklen_t worker_addrlen, int worker_sock_fd,
                              WorkerType* type, size_t* insert_index) {
    if (worker_addrlen != sizeof(*worker_addr)) {
        const struct sockaddr* base_addr =
            (const struct sockaddr*)worker_addr;
        fprintf(stderr, "Unknown worker of size %u\n", worker_addrlen);
        return false;
    }

    if (!receive_worker_type(worker_sock_fd, worker_addr, type)) {
        fprintf(stderr, "Could not get worker type at port %u\n",
                worker_addr->sin_port);
        return false;
    }

    const char* worker_type_str;
    switch (*type) {
        case FIRST_STAGE_WORKER:
            *insert_index = insert_new_worker_into_arrays(
                server->first_workers_addrs, server->first_workers_fds,
                &server->first_workers_arr_size,
                MAX_NUMBER_OF_FIRST_WORKERS, worker_addr, worker_sock_fd);
            worker_type_str = "first";
            break;
        case SECOND_STAGE_WORKER:
            *insert_index = insert_new_worker_into_arrays(
                server->second_workers_addrs, server->second_workers_fds,
                &server->second_workers_arr_size,
                MAX_NUMBER_OF_SECOND_WORKERS, worker_addr, worker_sock_fd);
            worker_type_str = "second";
            break;
        case THIRD_STAGE_WORKER:
            *insert_index = insert_new_worker_into_arrays(
                server->third_workers_addrs, server->third_workers_fds,
                &server->third_workers_arr_size,
                MAX_NUMBER_OF_THIRD_WORKERS, worker_addr, worker_sock_fd);
            worker_type_str = "third";
            break;
        default:
            fprintf(stderr, "Unknown worker type: %d\n", *type);
            return false;
    }

    if (*insert_index == (size_t)-1) {
        fprintf(stderr,
                "Can't accept new worker: limit for workers "
                "of type \"%s worker\" has been reached\n",
                worker_type_str);
        return false;
    }

    printf("Accepted new worker with type \"%s worker\"\n",
           worker_type_str);
    return true;
}

bool server_accept_worker(Server server, struct sockaddr_in* worker_addr,
                          WorkerType* type, size_t* insert_index) {
    socklen_t worker_addrlen = sizeof(*worker_addr);
    int worker_sock_fd       = accept(
        server->sock_fd, (struct sockaddr*)&worker_addr, &worker_addrlen);
    if (worker_sock_fd == -1) {
        perror("accept");
        return false;
    }
    if (!handle_new_worker(server, worker_addr, worker_addrlen,
                           worker_sock_fd, type, insert_index)) {
        close(worker_sock_fd);
        return false;
    }
    return true;
}
