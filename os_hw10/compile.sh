#! /bin/sh

gcc first_client.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -o first_client
gcc second_client.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -o second_client
gcc server.c -O2 -DNDEBUG -Wall -Wextra -Wpedantic -o server
