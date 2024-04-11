#! /bin/sh

gcc writer.c common.c -o writer -lpthread -lrt -o writer
gcc reader.c common.c -o reader -lpthread -lrt -o reader
