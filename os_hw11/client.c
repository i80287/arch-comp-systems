#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for printf, perror, size_t
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS
#include <sys/socket.h>  // for recvfrom
#include <unistd.h>      // for NULL, close, ssize_t

#include "client_config.h"  // for create_client_socket, report_invalid_command_args
#include "common_config.h"  // for MESSAGE_BUFFER_SIZE, is_end_message
#include "parser.h"         // for parse_port

static int run_client_impl(int sock_fd) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        ssize_t received_size =
            recvfrom(sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0, NULL, NULL);
        if (received_size <= 0) {
            perror("recvfrom");
            break;
        }

        const size_t message_string_length    = (size_t)received_size - 1;
        message_buffer[message_string_length] = '\0';
        received_end_message = is_end_message(message_buffer, message_string_length);
        printf("Received message '%s' of length %zu from the server\n", message_buffer,
               message_string_length);
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
