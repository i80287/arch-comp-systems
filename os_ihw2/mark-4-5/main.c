#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "config.h"
#include "parser.h"
#include "resources.h"
#include "workers.h"

static void wait_for_workers() {
    while (wait(NULL) != -1) {
    }
}

static int run_program(Resources* res, uint32_t first_workers,
                       uint32_t second_workers, uint32_t third_workers) {
    assert(res);

    if (start_first_workers(first_workers, res->first_to_second_sd.sh_mem) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (start_second_workers(second_workers, res->first_to_second_sd.sh_mem,
                             res->second_to_third_sd.sh_mem) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    if (start_third_workers(third_workers, res->second_to_third_sd.sh_mem) !=
        EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    signal(SIGINT, SIG_IGN);
    wait_for_workers();
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    ParseResult parse_res = parse_args(argc, argv);
    if (parse_res.status != PARSE_SUCCESS) {
        handle_invalid_args(parse_res, argv[0]);
        return EXIT_FAILURE;
    }

    Resources res;
    if (init_resources(&res) == RESOURCE_INIT_FAILURE) {
        return EXIT_FAILURE;
    }

    run_program(&res, parse_res.first_workers, parse_res.second_workers,
                parse_res.third_workers);

    deinit_resources(&res);
    return EXIT_SUCCESS;
}
