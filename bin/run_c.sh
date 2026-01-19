#!/bin/bash
# run_c.sh - Compile a .csm file to C and run it, cleaning up temporary files
# Usage: run_c.sh <source.csm>

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <source.csm>" >&2
    exit 1
fi

source_file="$1"

if [ ! -f "$source_file" ]; then
    echo "Error: File not found: $source_file" >&2
    exit 1
fi

# Get the directory containing this script
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
casm_bin="$script_dir/casm"

if [ ! -f "$casm_bin" ]; then
    echo "Error: casm binary not found at $casm_bin" >&2
    exit 1
fi

# Create a temporary directory for compilation artifacts
temp_dir=$(mktemp -d)
cleanup() {
    rm -rf "$temp_dir"
}
trap cleanup EXIT

# Compile .csm to C
generated_c="$temp_dir/generated.c"
if ! "$casm_bin" --target=c --output="$generated_c" "$source_file" > /dev/null 2>&1; then
    echo "Error: Compilation failed" >&2
    exit 1
fi

# Compile C to executable
executable="$temp_dir/executable"
if ! gcc -Wall -Wextra -pedantic -std=c99 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer "$generated_c" -o "$executable" -lm 2>&1; then
    echo "Error: C compilation failed" >&2
    exit 1
fi

# Run the executable
exec "$executable"
