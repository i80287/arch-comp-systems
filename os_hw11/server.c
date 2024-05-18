#include <arpa/inet.h>   // for htons
#include <netinet/in.h>  // for sockaddr_in
#include <stdbool.h>     // for bool, false
#include <stdint.h>      // for uint16_t
#include <stdio.h>       // for printf, fgets, perror, size_t, stdin
#include <stdlib.h>      // for EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>      // for strlen
#include <sys/socket.h>  // for sendto, AF_INET
#include <unistd.h>      // for close, ssize_t

#include "common_config.h"  // for MESSAGE_BUFFER_SIZE, is_end_message
#include "parser.h"         // for parse_args, ParseResult
#include "server_config.h"  // for create_server_socket, report_invalid_command_args

static int run_server_impl(int server_sock_fd, const struct sockaddr_in multicast_address) {
    char message_buffer[MESSAGE_BUFFER_SIZE] = {0};
    bool received_end_message                = false;
    while (!received_end_message && fgets(message_buffer, MESSAGE_BUFFER_SIZE, stdin)) {
        size_t message_size = strlen(message_buffer);
        if (message_buffer[message_size - 1] == '\n') {
            message_buffer[message_size - 1] = '\0';
            message_size--;
        }

        printf("Transfering message '%s' of length %zu from the first client to the second one\n",
               message_buffer, message_size);
        received_end_message = is_end_message(message_buffer, message_size);

        size_t send_size = message_size + 1;
        if (sendto(server_sock_fd, message_buffer, send_size, 0,
                   (const struct sockaddr*)&multicast_address,
                   sizeof(multicast_address)) != (ssize_t)send_size) {
            perror("send");
            break;
        }
    }

    printf("Server is stopping...\n");
    return received_end_message ? EXIT_SUCCESS : EXIT_FAILURE;
}

static int run_server(const char* multicast_ip, uint16_t multicast_port) {
    int sock_fd = create_server_socket();
    if (sock_fd == -1) {
        return EXIT_FAILURE;
    }
    const struct sockaddr_in broadcast_address = {
        .sin_family      = AF_INET,
        .sin_port        = htons(multicast_port),
        .sin_addr.s_addr = inet_addr(multicast_ip),
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

    return run_server(res.ip_address, res.port);
}
