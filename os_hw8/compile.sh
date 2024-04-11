#! /bin/sh

gcc writer.c common.c -o writer.out -lpthread -lrt
gcc reader.c common.c -o reader.out -lpthread -lrt
