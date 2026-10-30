#!/bin/sh
# Builds both programs into bin/. Run on GNU/Linux from the question1 folder.
# The binaries are only built here, never run.
set -e

mkdir -p bin
CFLAGS="-Wall -Wextra -Werror -pedantic -std=gnu89"

# Program A: no optimisation, full debug information, symbols kept
gcc $CFLAGS -O0 -g src/data_sync/*.c -o bin/data_sync

# Program B: optimised, no debug information, then stripped
gcc $CFLAGS -O2 src/data_sync_v2/*.c -o bin/data_sync_v2
strip --strip-all bin/data_sync_v2

gcc --version | head -n 1
ls -l bin
