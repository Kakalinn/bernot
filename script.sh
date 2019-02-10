#!/usr/bin/env bash
set -e
gcc test.c -o test > /dev/null
./test > /dev/null
