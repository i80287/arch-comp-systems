#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include "pin.h"

#if defined __GNUC__ && defined __GNUC_MINOR__ && ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((2) << 16) + (4))
#define FUNCTION_NAME __extension__ __PRETTY_FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif

static inline void app_perror_impl(const char* file, int line, const char* func_name) {
    int errno_val = errno;
    char buffer[512];
    snprintf(buffer, 512, "%s");
    errno = errno_val;
    perror(buffer);
}

#define app_perror() app_perror_impl(__FILE__, __LINE__, FUNCTION_NAME)

# ifdef	__USE_GNU
#  define assert_perror(errnum)						\
  (!(errnum)								\
   ? __ASSERT_VOID_CAST (0)						\
   : __assert_perror_fail ((errnum), __FILE__, __LINE__, __ASSERT_FUNCTION))
# endif

typedef enum ClientType {
    FIRST_STAGE_WORKER_CLIENT,
    SECOND_STAGE_WORKER_CLIENT,
    THIRD_STAGE_WORKER_CLIENT,
    // Invalid type, max value, e.t.c.
    CLIENT_TYPE_SENTINEL,
} ClientType;

static inline const char* client_type_to_string(ClientType type) {
    switch (type) {
        case FIRST_STAGE_WORKER_CLIENT:
            return "first stage worker";
        case SECOND_STAGE_WORKER_CLIENT:
            return "second stage worker";
        case THIRD_STAGE_WORKER_CLIENT:
            return "third stage worker";
        default:
            return "unknown client";
    }
}

typedef enum MessageType {
    PIN_TRANSFERRING,
    SHUTDOWN_MESSAGE,
} MessageType;

static inline const char* message_type_to_string(MessageType type) {
    switch (type) {
        case PIN_TRANSFERRING:
            return "pin transferring message";
        case SHUTDOWN_MESSAGE:
            return "shutdown message";
        default:
            return "unknown message";
    }
}

enum {
    UDP_MESSAGE_SIZE          = 512,
    UDP_MESSAGE_METAINFO_SIZE = sizeof(ClientType) + sizeof(MessageType),
    UDP_MESSAGE_BUFFER_SIZE   = UDP_MESSAGE_SIZE - UDP_MESSAGE_METAINFO_SIZE,
};

struct UDPMessage {
    MessageType message_type;
    ClientType source_client;
    char message_buffer[UDP_MESSAGE_BUFFER_SIZE];
};
