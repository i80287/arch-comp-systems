#pragma once

#include <ctype.h>    // for isdigit
#include <stdbool.h>  // for bool, false, true
#include <stddef.h>   // for size_t, NULL
#include <stdint.h>   // for uint16_t, uint8_t
#include <stdlib.h>   // for strtoul
#include <string.h>   // for strlen

typedef struct ParseResult {
    const char* ip_address;
    uint16_t port;
    bool succeeded;
} ParseResult;

static inline bool verify_ip(const char* ip_address) {
    const size_t max_pattern_length = sizeof("abc.def.ghi.jkz") - 1;
    const size_t ip_address_length  = strlen(ip_address);
    if (ip_address_length > max_pattern_length) {
        return false;
    }
    size_t dots_count = 0;
    for (size_t i = 0; i < ip_address_length; i++) {
        uint8_t chr = (uint8_t)ip_address[i];
        if (chr == '.') {
            dots_count++;
        } else if (!isdigit(chr)) {
            return false;
        }
    }
    return dots_count == 3;
}

static inline bool parse_port(const char* port_str, uint16_t* port) {
    char* end_ptr            = NULL;
    unsigned long port_value = strtoul(port_str, &end_ptr, 10);
    *port                    = (uint16_t)port_value;
    bool port_value_overflow = *port != port_value;
    bool parse_error         = end_ptr == NULL || *end_ptr != '\0';
    return !parse_error && !port_value_overflow;
}

static inline ParseResult parse_args(int argc, const char* argv[]) {
    ParseResult res = {.succeeded = false};
    if (argc != 3) {
        return res;
    }
    res.ip_address = argv[1];
    if (!verify_ip(res.ip_address)) {
        return res;
    }
    if (!parse_port(argv[2], &res.port)) {
        return res;
    }
    res.succeeded = true;
    return res;
}
