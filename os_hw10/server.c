#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for perror, printf
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>      // for memcmp
#include <sys/socket.h>  // for recv, send
#include <unistd.h>      // for close, size_t, ssize_t

#include "common_config.h"  // for END_MESSAGE_LENGTH, MESSAGE_BUFFER_SIZE
#include "parser.h"         // for parse_port
#include "server_config.h"  // for accept_client, configure_socket, create_server_socket

static int run_server_impl(int first_client_sock_fd, int second_client_sock_fd) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    do {
        const ssize_t received_size =
            recv(first_client_sock_fd, message_buffer, MESSAGE_BUFFER_SIZE, 0);
        if (received_size <= 0) {
            if (received_size < 0) {
                perror("recv");
            }
            break;
        }

        received_end_message = received_size == END_MESSAGE_LENGTH &&
                               memcmp(message_buffer, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
        const size_t send_size = (size_t)received_size;
        printf(
            "Transfering message '%s' of length %zu from the first client to the second one\n",
            message_buffer, send_size);
        if (send(second_client_sock_fd, message_buffer, send_size, 0) != (ssize_t)send_size) {
            perror("send");
            break;
        }
    } while (!received_end_message);

    printf("Server is stopping...");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int run_server(uint16_t server_port) {
    int ret                   = EXIT_FAILURE;
    int sock_fd               = -1;
    int first_client_sock_fd  = -1;
    int second_client_sock_fd = -1;

    sock_fd = create_server_socket();
    if (sock_fd == -1) {
        goto run_server_empty_cleanup_label;
    }
    if (!configure_socket(sock_fd, server_port)) {
        goto run_server_server_sock_cleanup_label;
    }
    first_client_sock_fd = accept_client(sock_fd);
    if (first_client_sock_fd == -1) {
        goto run_server_server_sock_cleanup_label;
    }
    second_client_sock_fd = accept_client(sock_fd);
    if (second_client_sock_fd == -1) {
        goto run_server_first_client_sock_cleanup_label;
    }

    ret = run_server_impl(first_client_sock_fd, second_client_sock_fd);

    close(second_client_sock_fd);
run_server_first_client_sock_cleanup_label:
    close(first_client_sock_fd);
run_server_server_sock_cleanup_label:
    close(sock_fd);
run_server_empty_cleanup_label:
    return ret;
}

int main(int argc, const char* argv[]) {
    uint16_t server_port;
    if (argc != 2 || !parse_port(argv[1], &server_port)) {
        report_invalid_command_args(argv[0]);
        return EXIT_FAILURE;
    }

    return run_server(server_port);
}
