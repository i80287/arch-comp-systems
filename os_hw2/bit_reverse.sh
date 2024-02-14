#!/bin/bash

function bit_reverse {
    declare -i n="$1"
    declare -i t=0
    ((n = (n << 31) | (n >> 33)))
    ((t = (n ^ (n >> 20)) & 0x00000FFF800007FF))
    ((n = (t | (t << 20)) ^ n))
    ((t = (n ^ (n >> 8)) & 0x00F8000F80700807))
    ((n = (t | (t << 8)) ^ n))
    ((t = (n ^ (n >> 4)) & 0x0808708080807008))
    ((n = (t | (t << 4)) ^ n))
    ((t = (n ^ (n >> 2)) & 0x1111111111111111))
    ((n = (t | (t << 2)) ^ n))
    echo "$n"
}

if [ $# -lt 1 ]; then
    echo "Usage: $(basename "$0") number"
    echo "Example: $(basename "$0") 2147483648"
    exit 1
fi

bit_reverse "$1"
