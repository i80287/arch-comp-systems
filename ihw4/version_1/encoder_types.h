#ifndef ENCODER_TYPES_H
#define ENCODER_TYPES_H 1

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "encoder_config.h"

typedef struct encoded_result {
    struct encoded_result* next;
    struct encoded_result* prev;
    size_t chunk_no;
    code_t encoded_string[ENCODE_CHUNK_SIZE_MACRO];
} encoded_result_t;

typedef struct multi_threads_package {
    pthread_mutex_t get_lock;
    size_t current_chunk_no;
    pthread_mutex_t store_lock;
    encoded_result_t* store_list_head;
    encoded_result_t* store_list_tail;
} package_t;

typedef struct threads_context {
    package_t package;
    const char* string;
    size_t string_length;
    pthread_mutex_t write_lock;
    FILE* output_file;
} threads_context_t;

static inline char* alloc_zero_str_with_check(size_t length) {
    // + 1 for '\0' termination symbol
    char* str = (char*)malloc(length + 1);
    if (unlikely(str == NULL)) {
        fputs("> Memory allocation failed\n", stderr);
    } else {
        memset((void*)str, 0, length + 1);
    }

    return str;
}

static inline void parse_input_file_arg(threads_context_t* ctx, const char* input_fname) {
    if (ctx->string != NULL) {
        fputs(
            "> No more than one input string can be passed for the encoding\n",
            stderr);
        return;
    }

    char* input_string = alloc_zero_str_with_check(MAX_INPUT_STRING_LEN);
    if (unlikely(input_string == NULL))
        return;

    FILE* fin = fopen(input_fname, "r");
    if (fin == NULL) {
        fprintf(stderr,
            "> Could not open file '%s' for reading\n",
            input_fname);
        free((void*)input_string);
        return;
    }

    bool read_error = fgets(input_string, (int)MAX_INPUT_STRING_LEN, fin) == NULL && !feof(fin);
    fclose(fin);

    if (read_error) {
        fprintf(stderr,
            "> An error occured while reading string from input file '%s'\n",
            input_fname);
        free((void*)input_string);
        return;
    }

    size_t string_length;
    char* newline_c = strchr(input_string, '\n');
    if (newline_c != NULL) {
        *newline_c = '\0';
        string_length = (size_t)(newline_c - input_string);
    }
    else {
        string_length = strlen(input_string);
    }

    ctx->string = input_string;
    ctx->string_length = string_length;
}

static inline void parse_output_file_arg(threads_context_t* ctx, const char* output_fname) {
    FILE* fout = fopen(output_fname, "w");
    if (fout == NULL) {
        fprintf(stderr,
            "> Could not open file '%s' for writing\n",
            output_fname);
        return;
    }

    ctx->output_file = fout;
}

static inline void parse_input_string_arg(threads_context_t* ctx, const char* arg_string) {
    if (ctx->string != NULL) {
        fputs(
            "> No more than one input string can be passed for the encoding\n",
            stderr);
        return;
    }

    size_t string_length = strlen(arg_string);
    char* input_string = alloc_zero_str_with_check(string_length);
    if (unlikely(input_string == NULL))
        return;

    memcpy(input_string, arg_string, string_length);
    ctx->string = input_string;
    ctx->string_length = string_length;
}

static inline void report_unknown_argument(const char* unknown_argument) {
    fprintf(stderr,
            "> Unknown flag '%s'\n"
            "> Currenty supported flags: '-fin', '-fout', '-in'\n",
            unknown_argument);
}

static inline void parse_args(threads_context_t* ctx, int argc, char* argv[]) {
    if (argc <= 1)
        return;

    size_t args_len = (size_t)argc;
    for (size_t i = 1; i + 1 < args_len; i += 2) {
        const char* argument = argv[i];
        if (strcmp(argument, "-fin") == 0) {
            parse_input_file_arg(ctx, argv[i + 1]);
        } else if (strcmp(argument, "-fout") == 0) {
            parse_output_file_arg(ctx, argv[i + 1]);
        } else if (strcmp(argument, "-in") == 0) {
            parse_input_string_arg(ctx, argv[i + 1]);
        } else {
            report_unknown_argument(argument);
        }
    }
}

static inline int32_t init_threads_context(threads_context_t* ctx, int argc,
                                           char* argv[]) {
    if (unlikely(ctx == NULL))
        return -1;

    memset((void*)ctx, 0, sizeof(threads_context_t));

    int res = pthread_mutex_init(&ctx->package.get_lock, NULL);
    if (unlikely(res != 0))
        return res;

    res = pthread_mutex_init(&ctx->package.store_lock, NULL);
    if (unlikely(res != 0)) {
        pthread_mutex_destroy(&ctx->package.get_lock);
        return res;
    }

    res = pthread_mutex_init(&ctx->write_lock, NULL);
    if (unlikely(res != 0)) {
        pthread_mutex_destroy(&ctx->package.store_lock);
        pthread_mutex_destroy(&ctx->package.get_lock);
        return res;
    }

    ctx->package.current_chunk_no = 0;
    ctx->package.store_list_head = NULL;
    ctx->package.store_list_tail = NULL;

    parse_args(ctx, argc, argv);

    if (ctx->string == NULL) {
        char* input_string = alloc_zero_str_with_check(MAX_INPUT_STRING_LEN);
        if (unlikely(input_string == NULL))
            return -1;

        printf("Input string of length no longer than %zu symbols\n> ",
            MAX_INPUT_STRING_LEN);
        // static_assert(MAX_INPUT_STRING_LEN == 2047);
        bool read_correctly = scanf("%2047s[^\n]", input_string) == 1;
        if (unlikely(!read_correctly)) {
            free((void*)input_string);
            puts("Wrong input");
            return -1;
        }

        ctx->string = input_string;
        ctx->string_length = strlen(input_string);
    }

    return 0;
}

static inline void deinit_threads_context(threads_context_t* ctx) {
    if (unlikely(ctx == NULL))
        return;

    if (likely(ctx->output_file != NULL))
        fclose(ctx->output_file);

    pthread_mutex_destroy(&ctx->package.store_lock);
    pthread_mutex_destroy(&ctx->package.get_lock);
    pthread_mutex_destroy(&ctx->write_lock);

    if (likely(ctx->string != NULL))
        free((void*)ctx->string);

    encoded_result_t* node = ctx->package.store_list_head;
    while (node != NULL) {
        encoded_result_t* next_node = node->next;
        free(node);
        node = next_node;
    }

    ctx->package.store_list_head = ctx->package.store_list_tail = NULL;
}

#endif // !ENCODER_TYPES_H
