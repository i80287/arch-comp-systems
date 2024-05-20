#! /bin/sh

gcc ./net/first-worker.c ./net/worker.c ./util/parser.c -g3 -fsanitize=address,undefined -D_FORTIFY_SOURCE=3 -fstack-protector-all -mshstk -O1 -Wall -Wextra -Wpedantic -Wsign-conversion -lrt -lm -o first-worker
gcc ./net/second-worker.c ./net/worker.c ./util/parser.c -g3 -fsanitize=address,undefined -D_FORTIFY_SOURCE=3 -fstack-protector-all -mshstk -O1 -Wall -Wextra -Wpedantic -Wsign-conversion -lrt -lm -o second-worker
gcc ./net/third-worker.c ./net/worker.c ./util/parser.c -g3 -fsanitize=address,undefined -D_FORTIFY_SOURCE=3 -fstack-protector-all -mshstk -O1 -Wall -Wextra -Wpedantic -Wsign-conversion -lrt -lm -o third-worker
gcc ./net/bridge-server.c ./net/server.c ./util/parser.c -g3 -fsanitize=address,undefined -D_FORTIFY_SOURCE=3 -fstack-protector-all -mshstk -O1 -Wall -Wextra -Wpedantic -Wsign-conversion -lrt -lpthread -o server
