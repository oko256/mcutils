#!/bin/bash

find "stm32" \
    -type d -path "*/test/build*" -prune -o \
    -type f \( -name "*.c" -o -name "*.h" \) \
    -exec clang-format-19 -i {} +
