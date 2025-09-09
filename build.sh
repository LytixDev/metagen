#!/bin/sh

#SRCS=$(find "src" -type f -name "*.c")
SRCS="src/error.c src/main.c src/lex.c src/type.c"
OUT="metagenc"

CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11"
#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -O3"
#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g -fsanitize=undefined"
#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g -fsanitize=address -fsanitize=undefined"

cc $CFLAGS $SRCS -o "$OUT"
