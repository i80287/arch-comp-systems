#! /bin/sh

clang-tidy ./net/*.c ./net/*.h ./util/*.c ./util/*.h -- -std=c11
