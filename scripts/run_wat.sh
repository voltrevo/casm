#!/bin/bash
# run_wat.sh - Compile a .csm file to WAT and run it, cleaning up temporary files
# Usage: run_wat.sh <source.csm>

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
project_root="$(dirname "$script_dir")"
executor_py="$project_root/tests/wat_executor.py"

if [ ! -f "$casm_bin" ]; then
    echo "Error: casm binary not found at $casm_bin" >&2
    exit 1
fi

if [ ! -f "$executor_py" ]; then
    echo "Error: WAT executor not found at $executor_py" >&2
    exit 1
fi

# Create a temporary directory for compilation artifacts
temp_dir=$(mktemp -d)
cleanup() {
    rm -rf "$temp_dir"
}
trap cleanup EXIT

# Compile .csm to WAT
generated_wat="$temp_dir/generated.wat"
if ! "$casm_bin" --target=wat --output="$generated_wat" "$source_file" > /dev/null 2>&1; then
    echo "Error: Compilation failed" >&2
    exit 1
fi

# Execute WAT with debug output
exec python3 "$executor_py" "$generated_wat"
