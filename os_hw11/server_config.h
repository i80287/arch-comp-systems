#pragma once

#include <netinet/in.h>  // for IPPROTO_UDP
#include <stdio.h>       // for perror, fprintf, stderr
#include <sys/socket.h>  // for setsockopt, socket, PF_INET, SOCK_DGRAM, SOL_SOCKET, SO_BROADCAST
#include <unistd.h>      // for close

static inline void report_invalid_command_args(const char* exec_path) {
    fprintf(stderr,
            "Usage: %s <multicast ip> <multicast port>\n"
            "Example: %s 127.0.0.1 8088\n",
            exec_path, exec_path);
}

static inline int create_server_socket() {
    int server_sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock_fd == -1) {
        perror("socket");
        return -1;
    }

    // Set TTL of multicast packet
    const int ttl = 64;
    if (setsockopt(server_sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, (const void*)&ttl, sizeof(ttl)) == -1) {
        perror("setsockopt");
        close(server_sock_fd);
        return -1;
    }

    return server_sock_fd;
}
