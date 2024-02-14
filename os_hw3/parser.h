#ifndef PARSER_H
#define PARSER_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    PARSE_OK         = 0,
    TOO_FEW_ARGS     = 1 << 1,
    TOO_MANY_ARGS    = 1 << 2,
    FIRST_ARG_ERROR  = 1 << 3,
    SECOND_ARG_ERROR = 1 << 4,
} parse_status_t;

typedef struct {
    uint32_t fib_num_index;
    uint32_t fact_index;
    parse_status_t status;
} parse_result_t;

parse_result_t parse_arguments(int argc, const char* const argv[]) {
    assert(argv);
    parse_result_t result = {
        .fib_num_index = 0,
        .fact_index    = 0,
        .status        = PARSE_OK,
    };

    if (argc != 3) {
        result.status = argc < 3 ? TOO_FEW_ARGS : TOO_MANY_ARGS;
        return result;
    }

    char* end_ptr        = NULL;
    long long res0       = strtoll(argv[1], &end_ptr, 10);
    result.fib_num_index = (uint32_t)res0;
    if (end_ptr == argv[1] || result.fib_num_index != res0 || errno == ERANGE) {
        // If no chars have been read
        //  or
        // If res0 can not fit into the uint32_t type
        //  or
        // If overflow occured during the conversion
        result.status |= FIRST_ARG_ERROR;
        // Clear errno
        errno = 0;
    }

    end_ptr           = NULL;
    long long res1    = strtoll(argv[2], &end_ptr, 10);
    result.fact_index = (uint32_t)res1;
    if (end_ptr == argv[2] || result.fact_index != res1 || errno == ERANGE) {
        /// If no chars have been read
        //  or
        // If res1 can not fit into the uint32_t type
        //  or
        // If overflow occured during the conversion
        result.status |= SECOND_ARG_ERROR;
        // Clear errno
        errno = 0;
    }

    return result;
}

void handle_parsing_error(parse_status_t status) {
    if (status & TOO_FEW_ARGS) {
        fputs("[>>>] Error: only 0 or 1 argument is provided\n", stderr);
    }
    if (status & TOO_MANY_ARGS) {
        fputs("[>>>] Error: more than 2 arguments are provided\n", stderr);
    }
    if (status & FIRST_ARG_ERROR) {
        fputs(
            "[>>>] Error: could not parse first argument. Expected integer in the "
            "interval [0; 4294967295]\n",
            stderr);
    }
    if (status & SECOND_ARG_ERROR) {
        fputs(
            "[>>>] Error: could not parse second argument. Expected integer in the "
            "interval [0; 4294967295]\n",
            stderr);
    }

    fputs(
        "[>>>] Usage hint   : executable-file-name num1 num2\n"
        "[>>>] Usage example: executable-file-name 10 20\n",
        stderr);
}

#endif  // !PARSER_H
