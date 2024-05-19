#pragma once

#include <stdint.h>
#include "../net/server.h"

typedef enum ParseStatus {
    PARSE_SUCCESS,
    PARSE_INVALID_ARGC,
    PARSE_INVALID_IP_ADDRESS,
    PARSE_INVALID_PORT,
} ParseStatus;

typedef struct ParseResultWorker1 {
    const char* ip_address;
    uint16_t port;
    ParseStatus status;
} ParseResultWorker1;

typedef struct ParseResultWorker2 {
    const char* first_ip_address;
    uint16_t first_port;
    const char* second_ip_address;
    uint16_t second_port;
    ParseStatus status;
} ParseResultWorker2;

typedef struct ParseResultWorker3 {
    const char* ip_address;
    uint16_t port;
    ParseStatus status;
} ParseResultWorker3;

typedef struct ParseResultServer {
    uint16_t port;
    ParseStatus status;
} ParseResultServer;

ParseResultWorker1 parse_args_worker_1(int argc, const char* argv[]);

ParseResultWorker2 parse_args_worker_2(int argc, const char* argv[]);

ParseResultWorker3 parse_args_worker_3(int argc, const char* argv[]);

ParseResultServer parse_args_server(int argc, const char* argv[]);

void print_invalid_args_error_worker_1(ParseStatus status,
                                       const char* program_path);

void print_invalid_args_error_worker_2(ParseStatus status,
                                       const char* program_path);

void print_invalid_args_error_worker_3(ParseStatus status,
                                       const char* program_path);

void print_invalid_args_error_server(ParseStatus status,
                                     const char* program_path);
                                     