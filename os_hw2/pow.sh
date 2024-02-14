#!/bin/bash

function bin_pow {
    declare -i n="$1"
    declare -i p="$2"
    declare -i res=1

    if ((p < 0)); then
        echo "exponent should be non-negative integer"
        return
    fi

    while true; do
        if ((p & 1)); then
            ((res *= n))
        fi

        ((p /= 2))
        if ((p == 0)); then
            echo "$res"
            return
        fi
        ((n *= n))
    done
}

if [ $# -lt 2 ]; then
    echo "Usage: $(basename "$0") pow_base exponent"
    echo "Example: $(basename "$0") 10 5"
    exit 1
fi

bin_pow "$@"
