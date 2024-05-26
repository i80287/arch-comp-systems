#! /bin/sh

gcc ./net/first-worker.c ./net/client-tools.c ./util/parser.c -g3 -O2 -Wall -Wextra -Wpedantic -Wsign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer -fstack-protector-all -mshstk -lrt -lm -o first-worker
gcc ./net/second-worker.c ./net/client-tools.c ./util/parser.c -g3 -O2 -Wall -Wextra -Wpedantic -Wsign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer -fstack-protector-all -mshstk -lrt -lm -o second-worker
gcc ./net/third-worker.c ./net/client-tools.c ./util/parser.c -g3 -O2 -Wall -Wextra -Wpedantic -Wsign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer -fstack-protector-all -mshstk -lrt -lm -o third-worker
gcc ./net/server.c ./net/server-tools.c ./util/parser.c -g3 -O2 -Wall -Wextra -Wpedantic -Wsign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer -fstack-protector-all -mshstk -lrt -lpthread -o server
