#! /bin/sh

gcc ./net/first-worker.c ./net/worker.c ./util/parser.c  -O2 -DNDEBUG -Wall -Wextra -Wpedantic -lrt -lm -o first-worker
gcc ./net/second-worker.c ./net/worker.c ./util/parser.c  -O2 -DNDEBUG -Wall -Wextra -Wpedantic -lrt -lm -o second-worker
gcc ./net/third-worker.c ./net/worker.c ./util/parser.c  -O2 -DNDEBUG -Wall -Wextra -Wpedantic -lrt -lm -o third-worker
