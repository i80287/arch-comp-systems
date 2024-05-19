#include <arpa/inet.h>   // for htons
#include <netinet/in.h>  // for sockaddr_in, IPPROTO_UDP
#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>   // for printf, fgets, perror, size_t, stdin, stderr
#include <stdlib.h>  // for EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>  // for strlen
#include <sys/socket.h>  // for sendto, AF_INET, setsockopt, socket, PF_INET, SOCK_DGRAM, SOL_SOCKET, SO_BROADCAST
#include <unistd.h>      // for close, ssize_t

#include "common_config.h"  // for MESSAGE_BUFFER_SIZE, is_end_message
#include "parser.h"         // for parse_args, ParseResult

static inline void report_invalid_command_args(const char* exec_path) {
    fprintf(stderr,
            "Usage: %s <ip address> <server port>\n"
            "Example: %s 127.0.0.1 8088\n",
            exec_path, exec_path);
}

static inline int create_server_socket() {
    int server_sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock_fd == -1) {
        perror("socket");
        return -1;
    }

    /* Set socket to allow broadcast */
    const int broadcast_permission = 1;
    int setsockopt_res             = setsockopt(
        server_sock_fd, SOL_SOCKET, SO_BROADCAST,
        (const void*)&broadcast_permission, sizeof(broadcast_permission));

    if (setsockopt_res == -1) {
        perror("setsockopt");
        close(server_sock_fd);
        return -1;
    }

    return server_sock_fd;
}

static int run_server_impl(int server_sock_fd,
                           const struct sockaddr_in broadcast_address) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    printf("Ready to send messages to the address:\n");
    print_socket_address_info(
        (const struct sockaddr_storage*)&broadcast_address,
        sizeof(broadcast_address));
    do {
        printf("Enter message:\n> ");
        if (fgets(message_buffer, MESSAGE_BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        size_t message_size = strlen(message_buffer);
        if (message_buffer[message_size - 1] == '\n') {
            message_buffer[message_size - 1] = '\0';
            message_size--;
        }

        received_end_message =
            is_end_message(message_buffer, message_size);
        size_t send_size = message_size + 1;
        if (sendto(server_sock_fd, message_buffer, send_size, 0,
                   (const struct sockaddr*)&broadcast_address,
                   sizeof(broadcast_address)) != (ssize_t)send_size) {
            perror("send");
            break;
        }

        printf(
            "Sent message \"%s\" length %zu to all listeners\n",
            message_buffer, message_size);
    } while (!received_end_message);

    printf("Server is stopping...\n");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int run_server(const char* server_ip, uint16_t server_port) {
    int sock_fd = create_server_socket();
    if (sock_fd == -1) {
        return EXIT_FAILURE;
    }
    const struct sockaddr_in broadcast_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(server_port),
        .sin_addr.s_addr = inet_addr(server_ip),
    };
    int ret = run_server_impl(sock_fd, broadcast_address);
    close(sock_fd);
    return ret;
}

int main(int argc, const char* argv[]) {
    ParseResult res = parse_args(argc, argv);
    if (!res.succeeded) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(res.server_ip, res.port);
}
