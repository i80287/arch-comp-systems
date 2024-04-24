#! /bin/sh

gcc first_worker.c resources.c -O2 -DNDEBUG -lrt -lpthread -lm -o first_worker
gcc second_worker.c resources.c -O2 -DNDEBUG -lrt -lpthread -lm -o second_worker
gcc third_worker.c resources.c -O2 -DNDEBUG -lrt -lpthread -lm -o third_worker
