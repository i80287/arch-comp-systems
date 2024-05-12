#pragma once

#include <arpa/inet.h>   // for htons, inet_addr
#include <netinet/in.h>  // for sockaddr_in, IPPROTO_TCP
#include <stdbool.h>     // for bool
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for perror, fprintf, stderr
#include <sys/socket.h>  // for connect, socket, AF_INET, PF_INET, SOCK_STREAM

static inline void report_invalid_command_args(const char* exec_path) {
    fprintf(stderr,
            "Usage: %s <Server Ip Address> <Server Port>\n"
            "Example: %s 192.168.0.0 8080\n",
            exec_path, exec_path);
}

static inline int create_client_socket() {
    int sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0) {
        perror("socket");
    }
    return sock_fd;
}

static inline bool connect_to_server(int sock_fd, const char* server_ip,
                                     uint16_t server_port) {
    const struct sockaddr_in server_socket_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(server_port),
        .sin_addr.s_addr = inet_addr(server_ip),
    };

    bool failed = connect(sock_fd, (const struct sockaddr*)&server_socket_address,
                          sizeof(server_socket_address)) == -1;
    if (failed) {
        perror("connect");
    }
    return !failed;
}
