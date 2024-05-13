#pragma once

#include <arpa/inet.h>  // for htonl, htons
#include <errno.h>
#include <netdb.h>       // for getnameinfo, gai_strerror, NI_NUMERICHOST
#include <netinet/in.h>  // for sockaddr_in, INADDR_ANY, IPPROTO_TCP
#include <stdbool.h>     // for bool, false, true
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for perror, fprintf, printf, stderr
#include <sys/socket.h>  // for accept, bind, listen, socket, socklen_t, AF_INET

enum { MAX_NUMBER_OF_CLIENTS = 2 };

static inline void report_invalid_command_args(const char* exec_path) {
    fprintf(stderr,
            "Usage: %s <Server Port>\n"
            "Example: %s 8088\n",
            exec_path, exec_path);
}

static inline void print_client_info(const struct sockaddr* socket_address,
                                     socklen_t addrlen) {
    char host_name[1024] = {0};
    char port_str[16]    = {0};
    int gai_err = getnameinfo(socket_address, addrlen, host_name, sizeof(host_name), port_str,
                              sizeof(port_str), NI_NUMERICHOST | NI_NUMERICSERV);
    if (gai_err != 0) {
        fprintf(stderr, "Could not fetch info about accepted client: %s\n",
                gai_strerror(gai_err));
    } else {
        printf(
            "Accepted client with address: %s\n"
            "Port: %s\n",
            host_name, port_str);
    }

    gai_err = getnameinfo(socket_address, addrlen, host_name, sizeof(host_name), port_str,
                          sizeof(port_str), 0);
    if (gai_err == 0) {
        printf("Readable address form: %s\nReadable port form: %s\n", host_name, port_str);
    }
}

static inline void print_server_info(const struct sockaddr* socket_address,
                                     socklen_t addrlen) {
    char host_name[1024] = {0};
    char port_str[16]    = {0};
    int gai_err = getnameinfo(socket_address, addrlen, host_name, sizeof(host_name), port_str,
                              sizeof(port_str), NI_NUMERICHOST | NI_NUMERICSERV);
    if (gai_err != 0) {
        fprintf(stderr, "Could not fetch info about current server: %s\n",
                gai_strerror(gai_err));
    } else {
        printf(
            "This server address: %s\n"
            "Port: %s\n",
            host_name, port_str);
    }
    gai_err = getnameinfo(socket_address, addrlen, host_name, sizeof(host_name), port_str,
                          sizeof(port_str), 0);
    if (gai_err == 0) {
        printf("Readable server address form: %s\nReadable port form: %s\n", host_name,
               port_str);
    }
}

static inline int create_server_socket(int16_t server_port) {
    int server_sock_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock_fd == -1) {
        perror("socket");
        return -1;
    }
    const struct sockaddr_in server_socket_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(server_port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    bool bind_failed = bind(server_sock_fd, (const struct sockaddr*)&server_socket_address,
                            sizeof(server_socket_address)) == -1;
    if (bind_failed) {
        perror("bind");
        goto create_server_socket_cleanup;
    }
    bool listen_failed = listen(server_sock_fd, MAX_NUMBER_OF_CLIENTS) == -1;
    if (listen_failed) {
        perror("listen");
        goto create_server_socket_cleanup;
    }
    print_server_info((const struct sockaddr*)&server_socket_address,
                      sizeof(server_socket_address));
    return server_sock_fd;
create_server_socket_cleanup:
    close(server_sock_fd);
    return -1;
}

static inline int accept_client(int server_sock_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int client_sock_fd =
        accept(server_sock_fd, (struct sockaddr*)&client_addr, &client_addrlen);
    if (client_sock_fd == -1) {
        perror("accept");
    } else {
        print_client_info((const struct sockaddr*)&client_addr, client_addrlen);
    }
    return client_sock_fd;
}
