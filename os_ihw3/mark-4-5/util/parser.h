#pragma once

#include <stdint.h> // for uint16_t

typedef enum ParseStatus {
    PARSE_SUCCESS,
    PARSE_INVALID_ARGC,
    PARSE_INVALID_IP_ADDRESS,
    PARSE_INVALID_PORT,
} ParseStatus;

typedef struct ParseResultWorker {
    const char* ip_address;
    uint16_t port;
    ParseStatus status;
} ParseResultWorker;

typedef struct ParseResultServer {
    uint16_t port;
    ParseStatus status;
} ParseResultServer;

ParseResultWorker parse_args_worker(int argc, const char* argv[]);

ParseResultServer parse_args_server(int argc, const char* argv[]);

void print_invalid_args_error_worker(ParseStatus status,
                                     const char* program_path);

void print_invalid_args_error_server(ParseStatus status,
                                     const char* program_path);
