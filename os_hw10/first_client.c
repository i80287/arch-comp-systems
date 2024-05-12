#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for printf, fgets, perror, size_t, NULL, stdin
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>      // for memcmp, strlen
#include <sys/socket.h>  // for send
#include <unistd.h>      // for close, ssize_t

#include "client_config.h"  // for connect_to_server, create_client_socket
#include "common_config.h"  // for END_MESSAGE_LENGTH, MESSAGE_BUFFER_SIZE
#include "parser.h"         // for parse_args, ParseResult

static int run_client_impl(int sock_fd, const char* server_ip, uint16_t server_port) {
    if (!connect_to_server(sock_fd, server_ip, server_port)) {
        return EXIT_FAILURE;
    }

    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    while (!received_end_message &&
           fgets(message_buffer, MESSAGE_BUFFER_SIZE, stdin) != NULL) {
        size_t input_length = strlen(message_buffer);
        if (input_length == 0) {
            continue;
        }
        if (message_buffer[input_length - 1] == '\n') {
            message_buffer[input_length - 1] = '\0';
            input_length--;
        }

        const size_t send_length = input_length + 1;
        if (send(sock_fd, message_buffer, send_length, 0) != (ssize_t)send_length) {
            perror("send");
            break;
        }

        printf("Sent message of size %zu to the server\n", input_length);
        received_end_message = input_length == END_MESSAGE_LENGTH &&
                               memcmp(message_buffer, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
    }

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
