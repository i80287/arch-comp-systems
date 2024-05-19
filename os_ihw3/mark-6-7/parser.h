#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

#include "config.h"

typedef enum ParseStatus {
    PARSE_SUCCESS,
    INVALID_ARGS_COUNT,
    FIRST_ARG_ERROR,
    SECOND_ARG_ERROR,
    THIRD_ARG_ERROR,
} ParseStatus;

typedef struct ParseResult {
    uint32_t first_workers;
    uint32_t second_workers;
    uint32_t third_workers;
    ParseStatus status;
} ParseResult;

ParseResult parse_args(int argc, char* argv[]);

void handle_invalid_args(ParseResult res, const char* program_path);

#endif
