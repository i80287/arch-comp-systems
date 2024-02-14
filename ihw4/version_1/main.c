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

static void write_result_log(threads_context_t* ctx,
                             const encoded_result_t* result,
                             const char* original_string, size_t encoded_size) {
    pthread_mutex_lock(&ctx->write_lock);

    printf("\n> Result of encoding substring %.*s:\n> ", (int)encoded_size,
           original_string);
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
    pthread_mutex_lock(&ctx->package.get_lock);
    size_t chunk_no = ctx->package.current_chunk_no++;
    pthread_mutex_unlock(&ctx->package.get_lock);
    return chunk_no;
}

static void store_result(threads_context_t* ctx, encoded_result_t* result) {
    if (unlikely(result == NULL)) {
        return;
    }

    pthread_mutex_lock(&ctx->package.store_lock);

    // Insert res so that we keep sorted order of the nodes
    encoded_result_t* current_node = ctx->package.store_list_tail;
    while (current_node != NULL) {
        if (current_node->chunk_no < result->chunk_no) {
            break;
        }

        current_node = current_node->prev;
    }

    if (current_node != NULL) {
        encoded_result_t* tmp_node = current_node->next;
        current_node->next = result;
        result->prev = current_node;

        result->next = tmp_node;
        if (tmp_node != NULL) {
            tmp_node->prev = result;
        } else {
            ctx->package.store_list_tail = result;
        }
    }
    else {
        // Linked list in package is empty or result->chunk_no is the lowest above all nodes
        encoded_result_t* tmp_node = ctx->package.store_list_head;
        ctx->package.store_list_head = result;
        result->next = tmp_node;
        if (unlikely(tmp_node == NULL)) {
            // List is empty, init tail
            ctx->package.store_list_tail = result;
        }
        else {
            tmp_node->prev = result;
        }
    }

    pthread_mutex_unlock(&ctx->package.store_lock);
}

static void print_encoding_results(const threads_context_t* ctx) {
    FILE* files[] = {stdout, ctx->output_file, NULL};
    FILE* outfile;
    for (size_t fj = 0; (outfile = files[fj]) != NULL; fj++) {
        fprintf(outfile,
                "\n> Result of encoding whole string %s:\n> ",
                ctx->string);
        for (const encoded_result_t* node = ctx->package.store_list_head;
             node != NULL; node = node->next) {
            for (size_t i = 0;
                 i < ENCODE_CHUNK_SIZE && node->encoded_string[i] != NO_CODE;
                 i++) {
                fprintf(outfile, "%u ", node->encoded_string[i]);
            }
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

        store_result(ctx, result);

        const char* original_str_start = ctx->string + chunk_no * ENCODE_CHUNK_SIZE;
        size_t encoded_size = ENCODE_CHUNK_SIZE - (size_t)(end_ptr - start_ptr);
        write_result_log(ctx, result, original_str_start, encoded_size);
    }

    return NULL;
}

static void run_threads(threads_context_t* ctx) {
    pthread_t threads[THREADS_SIZE];
    for (size_t i = 0; i < THREADS_SIZE; i++) {
        pthread_create(&threads[i], NULL, &encoder, (void*)ctx);
    }

    for (size_t i = 0; i < THREADS_SIZE; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char* argv[]) {
    threads_context_t ctx;
    if (unlikely(init_threads_context(&ctx, argc, argv) != 0)) {
        fputs("Failed to initialize context and locks\n", stderr);
        return EXIT_FAILURE;
    }

    run_threads(&ctx);
    print_encoding_results(&ctx);

    deinit_threads_context(&ctx);
    return EXIT_SUCCESS;
}
