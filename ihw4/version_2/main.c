#include "encoder_config.h"
#include "encoder_types.h"
#include "rand_sleep.h"

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

static void write_new_chunk_log(threads_context_t* ctx, size_t chunk_no) {
    pthread_mutex_lock(&ctx->write_lock);

    size_t first_pos = chunk_no * ENCODE_CHUNK_SIZE;
    size_t last_pos = first_pos + ENCODE_CHUNK_SIZE - 1;

    printf("> Starting encoding new chunk of the string (substr[%zu; %zu])\n",
        first_pos,
        last_pos);

    FILE* f = ctx->output_file;
    if (f != NULL) {
        fprintf(f, "> Starting encoding new chunk of the string (substr[%zu; %zu])\n",
            first_pos,
            last_pos);
    }

    pthread_mutex_unlock(&ctx->write_lock);
}

static void write_result_log(threads_context_t* ctx, const encoded_result_t* result, const char* original_string, size_t encoded_size) {
    pthread_mutex_lock(&ctx->write_lock);

    printf("\n> Result of encoding substring %.*s:\n> ", (int)encoded_size, original_string);
    for (size_t i = 0; i < encoded_size; i++) {
        printf("%u ", result->encoded_string[i]);
    }
    putchar('\n');

    FILE* f = ctx->output_file;
    if (f != NULL) {
        fprintf(f, "\n> Result of encoding substring %s:\n> ", original_string);
        for (size_t i = 0; i < encoded_size; i++) {
            fprintf(f, "%u ", result->encoded_string[i]);
        }
        fputc('\n', f);
    }

    pthread_mutex_unlock(&ctx->write_lock);
}

static size_t get_next_chunk_no(threads_context_t* ctx) {
    return ctx->package.last_free_chunk_no++;
}

static void store_result(package_t* pckg, encoded_result_t* result) {
    if (unlikely(result == NULL)) {
        return;
    }

    size_t index = pckg->current_array_size++;
    pckg->results_array[index] = result;
}

static int results_comparer(const void* first_arg, const void* second_arg) {
    const encoded_result_t* first_result =
        *(const encoded_result_t* const*)first_arg;
    const encoded_result_t* second_result =
        *(const encoded_result_t* const*)second_arg;

    return (int)(int64_t)(first_result->chunk_no - second_result->chunk_no);
}

static void sort_results(package_t* pckg) {
    qsort(pckg->results_array, pckg->current_array_size, sizeof(pckg->results_array[0]), &results_comparer);
}

static void print_encoding_results(const threads_context_t* ctx) {
    FILE* files[] = {stdout, ctx->output_file, NULL};
    FILE* outfile;
    for (size_t fj = 0; (outfile = files[fj]) != NULL; fj++) {
        fprintf(outfile,
                "\n> Result of encoding whole string %s:\n> ",
                ctx->string);

        encoded_result_t** results_array = ctx->package.results_array;
        size_t arr_size = ctx->package.current_array_size;
        for (size_t i = 0; i < arr_size; i++) {
            encoded_result_t* result = results_array[i];
            if (unlikely(result == NULL))
                continue;
            for (size_t j = 0; j < ENCODE_CHUNK_SIZE && result->encoded_string[j] != NO_CODE; j++)
                fprintf(outfile, "%u ", result->encoded_string[j]);
        }

        fputc('\n', outfile);
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

static void* encoder(void* arg) {
    threads_context_t* ctx = (threads_context_t*)arg;
    const unsigned char* string = (const unsigned char*)ctx->string;
    size_t string_length = ctx->string_length;

    const size_t max_chunk_no = (string_length + ENCODE_CHUNK_SIZE - 1) / ENCODE_CHUNK_SIZE;
    size_t chunk_no;
    while (likely((chunk_no = get_next_chunk_no(ctx)) < max_chunk_no)) {
        write_new_chunk_log(ctx, chunk_no);

        rand_sleep(MAX_SLEEP_SECONDS);

        encoded_result_t* result = new_result_node(chunk_no);
        if (unlikely(result == NULL)) {
            break;
        }

        const unsigned char* start_ptr = string + chunk_no * ENCODE_CHUNK_SIZE;
        const unsigned char* end_ptr = start_ptr + ENCODE_CHUNK_SIZE;
        for (code_t* write_p = result->encoded_string;
             start_ptr != end_ptr && *start_ptr != '\0';
             start_ptr++, write_p++) {
            *write_p = encode_char(*start_ptr);
        }

        store_result(&ctx->package, result);

        const char* original_str_start = ctx->string + chunk_no * ENCODE_CHUNK_SIZE;
        size_t encoded_size = ENCODE_CHUNK_SIZE - (size_t)(end_ptr - start_ptr);
        write_result_log(ctx, result, original_str_start, encoded_size);
    }

    return NULL;
}

static void run_threads(threads_context_t* ctx) {
    pthread_t threads[THREADS_SIZE];
    for (size_t i = 0; i < THREADS_SIZE; i++)
        pthread_create(&threads[i], NULL, &encoder, (void*)ctx);

    for (size_t i = 0; i < THREADS_SIZE; i++)
        pthread_join(threads[i], NULL);
}

int main(int argc, char* argv[]) {
    threads_context_t ctx;
    if (unlikely(init_threads_context(&ctx, argc, argv) != 0)) {
        fputs("Failed to initialize context and locks\n", stderr);
        return EXIT_FAILURE;
    }

    run_threads(&ctx);
    sort_results(&ctx.package);
    print_encoding_results(&ctx);

    deinit_threads_context(&ctx);
    return EXIT_SUCCESS;
}
