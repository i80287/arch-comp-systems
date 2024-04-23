#include "parser.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool parse_workers_number(const char* arg, uint32_t* workers_number,
                                 uint32_t min_workers_num,
                                 uint32_t max_workers_num) {
    char* end_ptr               = NULL;
    const int base              = 10;
    unsigned long first_workers = strtoul(arg, &end_ptr, base);
    if (end_ptr == NULL || *end_ptr != '\0') {
        return false;
    }

    if ((first_workers - min_workers_num) >
        (max_workers_num - min_workers_num)) {
        return false;
    }

    assert(workers_number);
    *workers_number = (uint32_t)first_workers;
    return true;
}

ParseResult parse_args(int argc, char* argv[]) {
    ParseResult result;
    memset(&result, 0, sizeof(result));
    result.status = PARSE_SUCCESS;

    if (argc != 4) {
        result.status = INVALID_ARGS_COUNT;
        return result;
    }

    if (!parse_workers_number(argv[1], &result.first_workers,
                              MIN_NUMBER_OF_FIRST_WORKERS,
                              MAX_NUMBER_OF_FIRST_WORKERS)) {
        result.status = FIRST_ARG_ERROR;
        return result;
    }

    if (!parse_workers_number(argv[2], &result.second_workers,
                              MIN_NUMBER_OF_SECOND_WORKERS,
                              MAX_NUMBER_OF_SECOND_WORKERS)) {
        result.status = SECOND_ARG_ERROR;
        return result;
    }

    if (!parse_workers_number(argv[3], &result.third_workers,
                              MIN_NUMBER_OF_THIRD_WORKERS,
                              MAX_NUMBER_OF_THIRD_WORKERS)) {
        result.status = THIRD_ARG_ERROR;
        return result;
    }

    return result;
}

void handle_invalid_args(ParseResult res, const char* program_path) {
    const char* error_str;
    switch (res.status) {
        case INVALID_ARGS_COUNT:
            error_str = "Invalid number of arguments";
            break;
        case FIRST_ARG_ERROR:
            error_str = "First argument is invalid";
            break;
        case SECOND_ARG_ERROR:
            error_str = "Second argument is invalid";
            break;
        case THIRD_ARG_ERROR:
            error_str = "Third argument is invalid";
            break;
        default:
            error_str = "Unknown error";
            break;
    }

    fprintf(stderr,
            "CLI args error: %s\n"
            "Usage: %s <number of first workers> <number of second workers> "
            "<number of third workers>\n"
            "Example: %s 1 1 1\n",
            error_str, program_path, program_path);
}
