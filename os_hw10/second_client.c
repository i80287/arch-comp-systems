#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for printf, perror
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS, size_t
#include <string.h>      // for memcmp
#include <sys/socket.h>  // for recv
#include <unistd.h>      // for close, ssize_t

#include "client_config.h"  // for connect_to_server, create_client_socket
#include "common_config.h"  // for END_MESSAGE_LENGTH, MESSAGE_BUFFER_SIZE
#include "parser.h"         // for parse_args, ParseResul

static int run_client_impl(int sock_fd, const char* server_ip, uint16_t server_port) {
    int res = EXIT_FAILURE;
    if (!connect_to_server(sock_fd, server_ip, server_port)) {
        return res;
    }

    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        const ssize_t received_size = recv(sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0);
        if (received_size <= 0) {
            perror("recv");
            break;
        }

        const size_t message_string_length = (size_t)received_size - 1;
        received_end_message = message_string_length == END_MESSAGE_LENGTH &&
                               memcmp(message_buffer, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
        printf("Received message '%s' of length %zu from the server\n", message_buffer,
               message_string_length);
    } while (!received_end_message);

    printf("Client is stopping...");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int run_client(const char* server_ip, uint16_t server_port) {
    int sock_fd = create_client_socket();
    if (sock_fd < 0) {
        return EXIT_FAILURE;
    }
    int res = run_client_impl(sock_fd, server_ip, server_port);
    close(sock_fd);
    return res;
}

int main(int argc, const char* argv[]) {
    ParseResult res = parse_args(argc, argv);
    if (!res.succeeded) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_client(res.server_ip, res.port);
}
