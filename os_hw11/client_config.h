#pragma once

#include <arpa/inet.h>   // for htonl, htons
#include <netinet/in.h>  // for sockaddr_in, INADDR_ANY, IPPROTO_UDP
#include <stdbool.h>     // for bool
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for perror, fprintf, stderr
#include <sys/socket.h>  // for bind, socket, AF_INET, PF_INET, SOCK_DGRAM
#include <unistd.h>      // for close

static inline void report_invalid_command_args(const char* exec_path) {
    fprintf(stderr,
            "Usage: %s <broadcast port>\n"
            "Example: %s 8088\n",
            exec_path, exec_path);
}

static inline int create_client_socket(uint16_t client_port) {
    int sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == -1) {
        perror("socket");
        return -1;
    }

    const struct sockaddr_in broadcast_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(client_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bool bind_failed =
        bind(sock_fd, (const struct sockaddr*)&broadcast_address, sizeof(broadcast_address)) == -1;
    if (bind_failed) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}
