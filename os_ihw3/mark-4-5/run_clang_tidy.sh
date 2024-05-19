#! /bin/sh

clang-tidy ./config/*.c ./config/*.h ./util/*.c ./util/*.h ./workers/*.c ./workers/*.h -- -std=c11
