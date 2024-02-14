#ifndef ENCODER_CONFIG_H
#define ENCODER_CONFIG_H 1

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t code_t;

// We should use macro as array size for
// the array in the struct encoded_result
#define ENCODE_CHUNK_SIZE_MACRO (128 * sizeof(char))
const size_t ENCODE_CHUNK_SIZE = ENCODE_CHUNK_SIZE_MACRO;
const code_t NO_CODE = (code_t)0;
const size_t NO_CHUNK = (size_t)-1;
const size_t THREADS_SIZE = 8;
const size_t MAX_INPUT_STRING_LEN = 2047;
const uint32_t MAX_SLEEP_SECONDS = 3;

#ifdef unlikely
#undef unlikely
#endif
#ifdef likely
#undef likely
#endif

#ifdef __GNUC__
#define unlikely(expr) __builtin_expect(expr, 0)
#define likely(expr)   __builtin_expect(expr, 1)
#else
#define unlikely(expr) (expr)
#define likely(expr)   (expr)
#endif

#endif // !ENCODER_CONFIG_H
