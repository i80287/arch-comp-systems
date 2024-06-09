#pragma once

#include <netdb.h>    // for getnameinfo, gai_strerror, NI_NUMERICHOST, NI_NUMERICSERV, NI_DGRAM
#include <stdbool.h>  // for bool
#include <string.h>   // for memcmp, size_t

enum { MESSAGE_BUFFER_SIZE = 256 };

static const char END_MESSAGE[] = "The End";
enum { END_MESSAGE_LENGTH = sizeof(END_MESSAGE) - 1 };

static inline bool is_end_message(const char* message,
                                  size_t message_size) {
    return message_size == END_MESSAGE_LENGTH &&
           memcmp(message, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
}

static inline void print_socket_address_info(
    const struct sockaddr_storage* socket_address_storage,
    socklen_t socket_address_size) {
    const struct sockaddr* socket_address =
        (const struct sockaddr*)socket_address_storage;
    char host_name[1024] = {0};
    char port_str[16]    = {0};
    int gai_err =
        getnameinfo(socket_address, socket_address_size, host_name,
                    sizeof(host_name), port_str, sizeof(port_str),
                    NI_NUMERICHOST | NI_NUMERICSERV | NI_DGRAM);
    if (gai_err == 0) {
        printf(
            "Socket address: %s\n"
            "Port: %s\n",
            host_name, port_str);
    } else {
        fprintf(stderr,
                "Could not fetch info about current socket address: %s\n",
                gai_strerror(gai_err));
    }

    gai_err = getnameinfo(socket_address, socket_address_size, host_name,
                          sizeof(host_name), port_str, sizeof(port_str),
                          NI_DGRAM);
    if (gai_err == 0) {
        printf(
            "Readable socket address form: %s\nReadable port form: %s\n",
            host_name, port_str);
    }
}
