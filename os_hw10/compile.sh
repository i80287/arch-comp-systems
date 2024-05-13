#! /bin/sh

gcc first_client.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wsign-conversion -o first_client
gcc second_client.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wsign-conversion -o second_client
gcc server.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Wsign-conversion -o server
