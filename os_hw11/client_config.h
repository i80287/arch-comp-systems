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
            "Usage: %s <multicast ip> <multicast port>\n"
            "Example: %s 127.0.0.1 8088\n",
            exec_path, exec_path);
}

static inline int create_client_socket(const char* multicast_ip, uint16_t multicast_port) {
    int sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd == -1) {
        perror("socket");
        goto create_client_socket_error_exit;
    }

    const struct sockaddr_in multicast_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(multicast_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bool bind_failed =
        bind(sock_fd, (const struct sockaddr*)&multicast_address, sizeof(multicast_address)) == -1;
    if (bind_failed) {
        perror("bind");
        goto create_client_socket_error_cleanup;
    }

    const struct ip_mreqn multicast_request = {
        // IP multicast address of group
        .imr_multiaddr.s_addr = inet_addr(multicast_ip),
        // Local IP address of interface: accept multicast from any interface
        .imr_address.s_addr = htonl(INADDR_ANY),
        // Interface index
        .imr_ifindex = 0,
    };
    bool setsockopt_failed = setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_IF, &multicast_request, sizeof(multicast_request));
    if (setsockopt_failed) {
        perror("setsockopt");
        goto create_client_socket_error_cleanup;
    }

    return sock_fd;
create_client_socket_error_cleanup:
    close(sock_fd);
create_client_socket_error_exit:
    return -1;
}
