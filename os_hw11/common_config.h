#pragma once

#include <stdbool.h>  // for bool
#include <string.h>   // for memcmp, size_t

enum { MESSAGE_BUFFER_SIZE = 256 };

static const char END_MESSAGE[] = "The End";
enum { END_MESSAGE_LENGTH = sizeof(END_MESSAGE) - 1 };

static inline bool is_end_message(const char* message, size_t message_size) {
    return message_size == END_MESSAGE_LENGTH &&
           memcmp(message, END_MESSAGE, END_MESSAGE_LENGTH) == 0;
}
