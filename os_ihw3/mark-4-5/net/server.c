#include "server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../util/config.h"

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
    if (pthread_mutex_init(&server->first_workers_mutex, NULL) != 0) {
        goto init_server_cleanup_empty;
    }
    if (pthread_mutex_init(&server->second_workers_mutex, NULL) != 0) {
        goto init_server_cleanup_mutex_1;
    }
    if (pthread_mutex_init(&server->third_workers_mutex, NULL) != 0) {
        goto init_server_cleanup_mutex_2;
    }
    server->sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->sock_fd == -1) {
        perror("socket");
        goto init_server_cleanup_mutex_3;
    }
    if (!setup_server(server->sock_fd, &server->sock_addr, server_port)) {
        goto init_server_cleanup_mutex_3_socket;
    }
    memset(server->first_workers_fds, -1,
           sizeof(int) * MAX_NUMBER_OF_FIRST_WORKERS);
    memset(server->second_workers_fds, -1,
           sizeof(int) * MAX_NUMBER_OF_SECOND_WORKERS);
    memset(server->third_workers_fds, -1,
           sizeof(int) * MAX_NUMBER_OF_THIRD_WORKERS);
    return true;

init_server_cleanup_mutex_3_socket:
    close(server->sock_fd);
init_server_cleanup_mutex_3:
    pthread_mutex_destroy(&server->third_workers_mutex);
init_server_cleanup_mutex_2:
    pthread_mutex_destroy(&server->second_workers_mutex);
init_server_cleanup_mutex_1:
    pthread_mutex_destroy(&server->first_workers_mutex);
init_server_cleanup_empty:
    return false;
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
    pthread_mutex_destroy(&server->third_workers_mutex);
    pthread_mutex_destroy(&server->second_workers_mutex);
    pthread_mutex_destroy(&server->first_workers_mutex);
}

static bool receive_worker_type(int worker_sock_fd, WorkerType* type) {
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

static void fill_worker_metainfo(WorkerMetainfo* info,
                                 const struct sockaddr_in* worker_addr) {
    int gai_err = getnameinfo((const struct sockaddr*)worker_addr,
                              sizeof(*worker_addr), info->host_name,
                              sizeof(info->host_name), info->port,
                              sizeof(info->port), 0);
    if (gai_err != 0) {
        fprintf(stderr,
                "> Could not fetch info about socket address: %s\n",
                gai_strerror(gai_err));
        strcpy(info->host_name, "unknown host");
        strcpy(info->port, "unknown port");
    }
}

static size_t insert_new_worker_into_arrays(
    struct sockaddr_in workers_addrs[], int workers_fds[],
    volatile size_t* array_size, const size_t max_array_size,
    WorkerMetainfo workers_info[], const struct sockaddr_in* worker_addr,
    int worker_sock_fd) {
    assert(*array_size <= max_array_size);
    for (size_t i = 0; i < max_array_size; i++) {
        if (workers_fds[i] != -1) {
            continue;
        }

        assert(array_size);
        assert(*array_size < max_array_size);
        assert(worker_addr);
        assert(workers_addrs);
        workers_addrs[i] = *worker_addr;
        assert(array_size);
        (*array_size)++;
        fill_worker_metainfo(&workers_info[i], worker_addr);
        assert(workers_fds);
        workers_fds[i] = worker_sock_fd;
        return i;
    }

    return (size_t)-1;
}

static bool handle_new_worker(Server server,
                              const struct sockaddr_storage* storage,
                              socklen_t worker_addrlen, int worker_sock_fd,
                              WorkerType* type, size_t* insert_index) {
    if (worker_addrlen != sizeof(struct sockaddr_in)) {
        fprintf(stderr, "> Unknown worker of size %u\n", worker_addrlen);
        return false;
    }

    // Enable sending of keep-alive messages
    // on connection-oriented sockets.
    uint32_t val = 1;
    if (setsockopt(worker_sock_fd, SOL_SOCKET, SO_KEEPALIVE, &val,
                   sizeof(val)) == -1) {
        perror("setsockopt");
        return false;
    }

    // The time (in seconds) the connection needs to remain idle
    // before TCP starts sending keepalive probes, if the socket
    // option SO_KEEPALIVE has been set on this socket.
    val = MAX_SLEEP_TIME;
    if (setsockopt(worker_sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, &val,
                   sizeof(val)) == -1) {
        perror("setsockopt");
        return false;
    }

    struct sockaddr_in worker_addr = {0};
    memcpy(&worker_addr, storage, sizeof(worker_addr));

    if (!receive_worker_type(worker_sock_fd, type)) {
        fprintf(stderr, "> Could not get worker type at port %u\n",
                worker_addr.sin_port);
        return false;
    }

    const char* worker_type_str;
    const WorkerMetainfo* info_array;
    switch (*type) {
        case FIRST_STAGE_WORKER: {
            if (!server_lock_first_mutex(server)) {
                return false;
            }
            *insert_index = insert_new_worker_into_arrays(
                server->first_workers_addrs, server->first_workers_fds,
                &server->first_workers_arr_size,
                MAX_NUMBER_OF_FIRST_WORKERS, server->first_workers_info,
                &worker_addr, worker_sock_fd);
            if (!server_unlock_first_mutex(server)) {
                return false;
            }
            worker_type_str = "first";
            info_array      = server->first_workers_info;
        } break;
        case SECOND_STAGE_WORKER: {
            if (!server_lock_second_mutex(server)) {
                return false;
            }
            *insert_index = insert_new_worker_into_arrays(
                server->second_workers_addrs, server->second_workers_fds,
                &server->second_workers_arr_size,
                MAX_NUMBER_OF_SECOND_WORKERS, server->second_workers_info,
                &worker_addr, worker_sock_fd);
            if (!server_unlock_second_mutex(server)) {
                return false;
            }
            worker_type_str = "second";
            info_array      = server->second_workers_info;
        } break;
        case THIRD_STAGE_WORKER: {
            if (!server_lock_third_mutex(server)) {
                return false;
            }
            *insert_index = insert_new_worker_into_arrays(
                server->third_workers_addrs, server->third_workers_fds,
                &server->third_workers_arr_size,
                MAX_NUMBER_OF_THIRD_WORKERS, server->third_workers_info,
                &worker_addr, worker_sock_fd);
            if (!server_unlock_third_mutex(server)) {
                return false;
            }
            worker_type_str = "third";
            info_array      = server->third_workers_info;
        } break;
        default:
            fprintf(stderr, "> Unknown worker type: %d\n", *type);
            return false;
    }

    if (*insert_index == (size_t)-1) {
        fprintf(stderr,
                "> Can't accept new worker: limit for workers "
                "of type \"%s worker\" has been reached\n",
                worker_type_str);
        return false;
    }

    printf(
        "> Accepted new worker with type \"%s worker\"(address=%s:%s)\n",
        worker_type_str, info_array[*insert_index].host_name,
        info_array[*insert_index].port);
    return true;
}

void send_shutdown_signal(int sock_fd, const WorkerMetainfo* info) {
    const char* host_name = info ? info->host_name : "unknown host";
    const char* port      = info ? info->port : "unknown port";
    if (send(sock_fd, SHUTDOWN_MESSAGE, SHUTDOWN_MESSAGE_SIZE, 0) !=
        SHUTDOWN_MESSAGE_SIZE) {
        perror("send");
        fprintf(stderr,
                "> Could not send shutdown signal to the "
                "worker[address=%s:%s]\n",
                host_name, port);
    } else {
        printf("> Sent shutdown signal to the worker[address=%s:%s]\n",
               host_name, port);
    }
}

bool server_accept_worker(Server server, WorkerType* type,
                          size_t* insert_index) {
    struct sockaddr_storage storage;
    socklen_t worker_addrlen = sizeof(storage);
    int worker_sock_fd       = accept(
        server->sock_fd, (struct sockaddr*)&storage, &worker_addrlen);
    if (worker_sock_fd == -1) {
        if (errno != EINTR) {
            perror("accept");
        }
        return false;
    }
    if (!handle_new_worker(server, &storage, worker_addrlen,
                           worker_sock_fd, type, insert_index)) {
        send_shutdown_signal(worker_sock_fd, NULL);
        close(worker_sock_fd);
        return false;
    }
    return true;
}
