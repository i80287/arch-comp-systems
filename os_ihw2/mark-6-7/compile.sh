#! /bin/sh

gcc main.c parser.c resources.c workers.c -O2 -DNDEBUG -lrt -lpthread -lm -o app
