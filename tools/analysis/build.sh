#!/bin/sh
# Build the offline VST2 host as x86_64 (the original plugin has no arm64 slice).
# Run it under Rosetta: arch -x86_64 build/vsthost ...
set -e
cd "$(dirname "$0")"
mkdir -p build
clang -O2 -arch x86_64 -framework CoreFoundation vsthost.c -o build/vsthost
echo "built build/vsthost"
