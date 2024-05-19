#include "worker.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static bool send_worker_type_info(int socket_fd,
                                  struct sockaddr_in* server_sock_addr,
                                  WorkerType type) {
    union {
        char bytes[sizeof(type)];
        WorkerType type;
    } buffer;
    buffer.type = type;
    if (sendto(socket_fd, buffer.bytes, sizeof(buffer.bytes), 0,
               (const struct sockaddr*)server_sock_addr,
               sizeof(*server_sock_addr)) != sizeof(buffer.bytes)) {
        perror("sendto[while sending worker type to the server]");
        return false;
    }

    return true;
}

static bool connect_to_server(int socket_fd,
                              struct sockaddr_in* server_sock_addr,
                              const char* server_ip, uint16_t server_port,
                              WorkerType type) {
    server_sock_addr->sin_family      = AF_INET;
    server_sock_addr->sin_port        = htons(server_port);
    server_sock_addr->sin_addr.s_addr = inet_addr(server_ip);

    bool failed =
        connect(socket_fd, (const struct sockaddr*)server_sock_addr,
                sizeof(*server_sock_addr)) == -1;
    if (failed) {
        perror("connect");
        return false;
    }

    if (!send_worker_type_info(socket_fd, server_sock_addr, type)) {
        fprintf(stderr,
                "Could not send self type %d to the server %s:%u\n", type,
                server_ip, server_port);
        return false;
    }

    return true;
}

static int create_socket() {
    int sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd == -1) {
        perror("socket");
    }
    return sock_fd;
}

bool init_single_server_worker(SingleServerWorker worker,
                               const char* server_ip, uint16_t server_port,
                               WorkerType type) {
    worker->worker_sock_fd = create_socket();
    if (worker->worker_sock_fd == -1) {
        return false;
    }

    bool connected = connect_to_server(worker->worker_sock_fd,
                                       &worker->server_sock_addr,
                                       server_ip, server_port, type);
    if (!connected) {
        close(worker->worker_sock_fd);
    }

    return connected;
}

void deinit_single_server_worker(SingleServerWorker worker) {
    close(worker->worker_sock_fd);
}

static void print_sock_addr_info(const struct sockaddr_in* sock_addr) {
    const struct sockaddr* socket_address =
        (const struct sockaddr*)sock_addr;
    const socklen_t socket_address_size = sizeof(*sock_addr);
    char host_name[1024]                = {0};
    char port_str[16]                   = {0};
    int gai_err =
        getnameinfo(socket_address, socket_address_size, host_name,
                    sizeof(host_name), port_str, sizeof(port_str),
                    NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM);
    if (gai_err == 0) {
        printf(
            "Numeric server socket address: %s\n"
            "Numeric server Port: %s\n",
            host_name, port_str);
    } else {
        fprintf(stderr, "Could not fetch info about socket address: %s\n",
                gai_strerror(gai_err));
    }

    gai_err = getnameinfo(socket_address, socket_address_size, host_name,
                          sizeof(host_name), port_str, sizeof(port_str),
                          NI_DGRAM);
    if (gai_err == 0) {
        printf("Server socket address: %s\nServer port: %s\n", host_name,
               port_str);
    }
}

void print_single_server_worker_info(SingleServerWorker worker) {
    print_sock_addr_info(&worker->server_sock_addr);
}

bool init_dual_server_worker(DualServerWorker worker,
                             const char* first_server_ip,
                             uint16_t first_server_port,
                             const char* second_server_ip,
                             uint16_t second_server_port,
                             WorkerType type) {
    worker->worker_sock_fd = create_socket();
    if (worker->worker_sock_fd == -1) {
        return false;
    }

    bool connected =
        connect_to_server(worker->worker_sock_fd,
                          &worker->first_server_sock_addr, first_server_ip,
                          first_server_port, type) &&
        connect_to_server(worker->worker_sock_fd,
                          &worker->second_server_sock_addr,
                          second_server_ip, second_server_port, type);
    if (!connected) {
        close(worker->worker_sock_fd);
    }

    return connected;
}

void deinit_dual_server_worker(DualServerWorker worker) {
    close(worker->worker_sock_fd);
}

void print_dual_server_worker_info(DualServerWorker worker) {
    printf("First server address\n");
    print_sock_addr_info(&worker->first_server_sock_addr);
    printf("Second server address\n");
    print_sock_addr_info(&worker->second_server_sock_addr);
}
