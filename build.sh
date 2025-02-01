#!/bin/sh

SRCS=$(find "src" -type f -name "*.c")
OUT="metagenc"

#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -03"
CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g"
#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g -fsanitize=address -fsanitize=undefined"

cc $CFLAGS $SRCS -o "$OUT"
