#! /bin/sh

gcc client.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wsign-conversion -o client
gcc server.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wsign-conversion -o server
