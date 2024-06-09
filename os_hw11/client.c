#include <arpa/inet.h>   // for htonl, htons
#include <netinet/in.h>  // for sockaddr_in, INADDR_ANY, IPPROTO_UDP
#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for printf, perror, fprintf, stderr, size_t
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS
#include <sys/socket.h>  // for bind, socket, AF_INET, PF_INET, SOCK_DGRAM, recvfrom
#include <unistd.h>  // for NULL, close, ssize_t

#include "common_config.h"  // for MESSAGE_BUFFER_SIZE, is_end_message
#include "parser.h"         // for parse_port

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
        bind(sock_fd, (const struct sockaddr*)&broadcast_address,
             sizeof(broadcast_address)) == -1;
    if (bind_failed) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    printf("Client ready to read messages from the address:\n");
    print_socket_address_info(
        (const struct sockaddr_storage*)&broadcast_address,
        sizeof(broadcast_address));

    return sock_fd;
}

static void print_received_message_with_info(
    const char message[MESSAGE_BUFFER_SIZE], size_t message_length,
    const struct sockaddr_storage* broadcast_address_storage,
    socklen_t broadcast_address_size) {
    printf("Received message '%s' of length %zu from the address:\n",
           message, message_length);
    print_socket_address_info(broadcast_address_storage,
                              broadcast_address_size);
}

static int run_client_impl(int sock_fd) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        struct sockaddr_storage broadcast_address_storage;
        socklen_t broadcast_address_size =
            sizeof(broadcast_address_storage);
        ssize_t received_size =
            recvfrom(sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0,
                     (struct sockaddr*)&broadcast_address_storage,
                     &broadcast_address_size);

        if (received_size < 0) {
            perror("recvfrom");
            break;
        }

        const size_t message_string_length    = (size_t)received_size - 1;
        message_buffer[message_string_length] = '\0';
        received_end_message =
            is_end_message(message_buffer, message_string_length);

        print_received_message_with_info(
            message_buffer, message_string_length,
            &broadcast_address_storage, broadcast_address_size);
    } while (!received_end_message);

    printf("Client is stopping...\n");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int run_client(uint16_t client_port) {
    int sock_fd = create_client_socket(client_port);
    if (sock_fd == -1) {
        return EXIT_FAILURE;
    }
    int res = run_client_impl(sock_fd);
    close(sock_fd);
    return res;
}

int main(int argc, const char* argv[]) {
    uint16_t client_port;
    if (argc != 2 || !parse_port(argv[1], &client_port)) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_client(client_port);
}
