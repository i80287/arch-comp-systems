#! /bin/sh

clang-format -i ./net/*.c ./net/*.h ./util/*.c ./util/*.h -fallback-style=Google
