#ifndef PARSER_H
#define PARSER_H

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    PARSE_OK                = 0,
    TOO_FEW_ARGS            = 1 << 1,
    TOO_MANY_ARGS           = 1 << 2,
    START_INDEX_ERROR       = 1 << 3,
    END_INDEX_ERROR         = 1 << 4,
    INCORRECT_INDEXES_ORDER = 1 << 5
} parse_status_t;

typedef struct {
    const char* input_filename;
    const char* output_filename;
    uint32_t start_n1;
    uint32_t end_n2;
    parse_status_t status;
} parse_result_t;

parse_result_t parse_arguments(int argc, const char* const argv[]) {
    assert(argv);
    parse_result_t result = { 0 };

    if (argc != 5) {
        result.status = argc < 5 ? TOO_FEW_ARGS : TOO_MANY_ARGS;
        return result;
    }

    result.input_filename  = argv[1];
    result.output_filename = argv[2];

    char* end_ptr          = NULL;
    long long res_start_n1 = strtoll(argv[3], &end_ptr, 10);
    result.start_n1        = (uint32_t)res_start_n1;
    if (end_ptr == argv[3] || result.start_n1 != res_start_n1 || errno == ERANGE) {
        // If no chars have been read
        //  or
        // If res_start_n1 can not fit into the uint32_t type
        //  or
        // If overflow occured during the conversion
        result.status = (parse_status_t)((uint32_t)result.status | START_INDEX_ERROR);
        // Clear errno
        errno = 0;
    }

    end_ptr              = NULL;
    long long res_end_n2 = strtoll(argv[4], &end_ptr, 10);
    result.end_n2        = (uint32_t)res_end_n2;
    if (end_ptr == argv[4] || result.end_n2 != res_end_n2 || errno == ERANGE) {
        /// If no chars have been read
        //  or
        // If res_end_n2 can not fit into the uint32_t type
        //  or
        // If overflow occured during the conversion
        result.status = (parse_status_t)((uint32_t)result.status | END_INDEX_ERROR);
        // Clear errno
        errno = 0;
    }

    if (result.start_n1 > result.end_n2) {
        result.status = (parse_status_t)((uint32_t)result.status | INCORRECT_INDEXES_ORDER);
    }

    return result;
}

void handle_parsing_error(parse_status_t status) {
    if (status & TOO_FEW_ARGS) {
        fputs("[>>>] Error: too few is provided (4 expected)\n", stderr);
    }
    if (status & TOO_MANY_ARGS) {
        fputs("[>>>] Error: too many arguments are provided (4 expected)\n", stderr);
    }
    if (status & START_INDEX_ERROR) {
        fputs(
            "[>>>] Error: could not parse start index. Expected integer in the "
            "interval [0; 4294967295]\n",
            stderr);
    }
    if (status & END_INDEX_ERROR) {
        fputs(
            "[>>>] Error: could not parse end index. Expected integer in the "
            "interval [0; 4294967295]\n",
            stderr);
    }
    if (status & INCORRECT_INDEXES_ORDER) {
        fputs("[>>>] Error: start index should be <= than end index\n", stderr);
    }

    fputs(
        "[>>>] Usage hint   : executable input-file-name output-file-name num1 num2\n"
        "[>>>] Usage example: main.out in.txt out.txt 5 10\n"
        "[>>>] Start index should be >= 0, end index (included) should be >= start "
        "index\n",
        stderr);
}

#endif  // !PARSER_H
