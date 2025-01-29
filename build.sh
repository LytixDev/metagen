#!/bin/sh

SRCS=$(find "src" -type f -name "*.c")
OUT="metagenc"

#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -03"
CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g"
#CFLAGS="-Isrc -Wall -Wpedantic -Wextra -Wshadow -std=c11 -g -fsanitize=address -fsanitize=undefined"


if [ "$1" = "parser" ] 
then
    # Build standalone parser
    CFLAGS="$CFLAGS -DBUILD_STANDALONE_PARSER"
    OUT="metagenc-parser"
fi

cc $CFLAGS $SRCS -o "$OUT"

# Copy parser to test/parser_e2e
if [ "$1" = "parser" ] 
then
    cp $OUT "test/parser_e2e"
fi
