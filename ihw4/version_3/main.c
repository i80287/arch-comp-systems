#include "encoder_config.h"
#include "encoder_types.h"
#include "rand_sleep.h"

#include <omp.h>

static code_t encode_char(int c) {
    static const code_t coding_table[] = {
        40, // 'A'
        35,
        164,
        65,
        88,
        95,
        26,
        3,
        159,
        190,
        25,
        19,
        53,
        184,
        211,
        157,
        121,
        163,
        168,
        137,
        42,
        17,
        93,
        145,
        242,
        36, // 'Z'
        NO_CODE,
        NO_CODE,
        NO_CODE,
        NO_CODE,
        NO_CODE,
        NO_CODE,
        158, // 'a'
        128,
        148,
        155,
        102,
        210,
        100,
        68,
        71,
        207,
        175,
        122,
        60,
        198,
        9,
        139,
        108,
        81,
        1,
        169,
        195,
        194,
        193,
        61,
        185,
        140, // 'z'
    };

    /**
     * Cast to uint8_t to ensure that possible
     * sign extension of 'c' wont affect answer.
     * Cast to uint32_t to force zero extension
     * (by default uint8_t will be casted to int
     *  in arithmetic operation)
     */
    size_t index = (uint32_t)(uint8_t)c - 'A';
    if (unlikely(index >= sizeof(coding_table) / sizeof(coding_table[0]))) {
        // i.e. c is not in range ['a'; 'Z']
        return NO_CODE;
    }

    return coding_table[index];
}

static int results_comparer(const void* first_arg, const void* second_arg) {
    const encoded_result_t* first_result =
        *(const encoded_result_t* const*)first_arg;
    const encoded_result_t* second_result =
        *(const encoded_result_t* const*)second_arg;

    return (int)(int64_t)(first_result->chunk_no - second_result->chunk_no);
}

static void print_encoding_results(const encoded_result_t* const* results_array,
                                   size_t arr_size,
                                   const threads_context_t* ctx) {
    FILE* files[] = {stdout, ctx->output_file, NULL};
    FILE* outfile;
    for (size_t fj = 0; (outfile = files[fj]) != NULL; fj++) {
        fprintf(outfile,
                "\n> Result of encoding whole string %s:\n> ",
                ctx->string);

        for (size_t i = 0; i < arr_size; i++) {
            const encoded_result_t* result = results_array[i];
            if (unlikely(result == NULL))
                continue;
            for (size_t j = 0; j < ENCODE_CHUNK_SIZE && result->encoded_string[j] != NO_CODE; j++)
                fprintf(outfile, "%u ", result->encoded_string[j]);
        }

        fputc('\n', outfile);
    }
}

static void write_new_chunk_log(size_t chunk_no, FILE* output_file) {
    size_t first_pos = chunk_no * ENCODE_CHUNK_SIZE;
    size_t last_pos = first_pos + ENCODE_CHUNK_SIZE - 1;
    printf("> Starting encoding new chunk of the string (substr[%zu; %zu])\n",
           first_pos, last_pos);

    if (output_file != NULL) {
        fprintf(
            output_file,
            "> Starting encoding new chunk of the string (substr[%zu; %zu])\n",
            first_pos, last_pos);
    }
}

static void write_result_log(const encoded_result_t* result,
                                size_t encoded_size,
                                const char* original_string,
                                FILE* output_file) {
    printf("\n> Result of encoding substring %.*s:\n> ", (int)encoded_size,
           original_string);
    for (size_t i = 0; i < encoded_size; i++) {
        printf("%u ", result->encoded_string[i]);
    }
    putchar('\n');

    if (output_file != NULL) {
        fprintf(output_file, "\n> Result of encoding substring %.*s:\n> ",
                (int)encoded_size, original_string);
        for (size_t i = 0; i < encoded_size; i++) {
            fprintf(output_file, "%u ", result->encoded_string[i]);
        }
        fputc('\n', output_file);
    }
}

static encoded_result_t* new_result_node(size_t chunk_no) {
    encoded_result_t* node =
        (encoded_result_t*)malloc(sizeof(encoded_result_t));
    if (unlikely(node == NULL)) {
        fprintf(stderr,
                "> Could not allocate memory for storing the %zu encoded chunk of string\n",
                chunk_no);
        return NULL;
    }

    // static_assert(NO_CODE == 0);
    memset((void*)node, 0, sizeof(encoded_result_t));
    node->chunk_no = chunk_no;
    return node;
}

int main(int argc, char* argv[]) {
    threads_context_t ctx;
    if (unlikely(init_threads_context(&ctx, argc, argv) != 0)) {
        fputs("Failed to initialize context and locks\n", stderr);
        return EXIT_FAILURE;
    }

    encoded_result_t** const results_array = (encoded_result_t**)malloc(sizeof(encoded_result_t*) * ctx.string_length);
    if (unlikely(results_array == NULL)) {
        fputs("Memory allocation for the encoded string storage failed\n", stderr);
        deinit_threads_context(&ctx);
        return EXIT_FAILURE;
    }

    // Show to the compiler that fields of the ctx are const
    const char* const input_string = ctx.string;
    const size_t string_length = ctx.string_length;
    const size_t max_chunk_no = (string_length + ENCODE_CHUNK_SIZE - 1) / ENCODE_CHUNK_SIZE;
    FILE* const output_file = ctx.output_file;

    size_t chunk_no = 0;
    size_t current_array_size = 0;

    #pragma omp parallel
    {
        while (true) {
            size_t local_chunk_no = 0;
            #pragma omp critical
            local_chunk_no = chunk_no++;

            if (unlikely(local_chunk_no >= max_chunk_no)) {
                break;
            }

            #pragma omp critical
            write_new_chunk_log(local_chunk_no, output_file);

            rand_sleep(MAX_SLEEP_SECONDS);

            encoded_result_t* const result = new_result_node(local_chunk_no);
            if (unlikely(result == NULL)) {
                break;
            }

            const unsigned char* start_ptr = (const unsigned char*)input_string + local_chunk_no * ENCODE_CHUNK_SIZE;
            const unsigned char* const end_ptr = start_ptr + ENCODE_CHUNK_SIZE;
            for (code_t* write_p = result->encoded_string;
                start_ptr != end_ptr && *start_ptr != '\0';
                start_ptr++, write_p++) {
                *write_p = encode_char(*start_ptr);
            }

            size_t index = 0;
            #pragma omp critical
            index = current_array_size++;

            results_array[index] = result;

            const char* const original_string = input_string + local_chunk_no * ENCODE_CHUNK_SIZE;
            const size_t encoded_size = ENCODE_CHUNK_SIZE - (size_t)(end_ptr - start_ptr);
            #pragma omp critical
            write_result_log(result, encoded_size, original_string, output_file);
        }
    }

    qsort(results_array,
        current_array_size,
        sizeof(results_array[0]),
        &results_comparer);

    print_encoding_results((const encoded_result_t* const*)results_array,
                           current_array_size, &ctx);

    deinit_threads_context(&ctx);
    return EXIT_SUCCESS;
}
