#!/bin/bash
# DBG test runner - tests the dbg() debugging feature
# Each test case is a directory containing:
#   test.csm - the source file to compile
#   output.txt - expected program output (stdout+stderr)
#   out.c - expected generated C code
#   out.wat - expected generated WAT code
#
# Tests can be either:
#   1. Success case: compile succeeds, generated code matches expected, program output matches expected
#   2. Error case: compile fails and error output matches expected

set -e

DBG_TEST_TIMEOUT=2
CASM_BIN="../../../bin/casm"
PASSED=0
FAILED=0

# Save original directory
ORIG_DIR=$(pwd)

if [ ! -f "$ORIG_DIR/bin/casm" ]; then
    echo "✗ bin/casm binary not found. Run 'make' first."
    exit 1
fi

# Wasmtime is required for validating WAT code execution
if ! command -v wasmtime &> /dev/null; then
    echo "✗ wasmtime is required but not found."
    echo "  wasmtime is needed to validate WebAssembly Text (WAT) code generation and execution."
    echo ""
    echo "  Installation options:"
    echo "    1. Visit https://docs.wasmtime.dev/cli-installing.html for installation instructions"
    echo "    2. Or run: curl https://wasmtime.dev/install.sh -sSf | bash"
    echo ""
    echo "  After installation, ensure 'wasmtime' is in your PATH."
    exit 1
fi

# Find all dbg test directories
for test_dir in tests/dbg_cases/*/; do
    test_name=$(basename "$test_dir")
    test_file="${test_dir%/}/test.csm"
    output_file="${test_dir%/}/output.txt"
    expected_c_file="${test_dir%/}/out.c"
    expected_wat_file="${test_dir%/}/out.wat"
    
    if [ ! -f "$test_file" ]; then
        echo "  Skipping $test_name (no test.csm)"
        continue
    fi
    
    if [ ! -f "$output_file" ]; then
        echo "  Skipping $test_name (no output.txt)"
        continue
    fi
    
    echo -n "  Testing $test_name... "
    
    # Create a temporary directory for this test's artifacts
    temp_dir=$(mktemp -d)
    cleanup() { rm -rf "$temp_dir"; }
    trap cleanup EXIT
    
    # Change to test directory so the filename in error messages is just "test.csm"
    cd "$ORIG_DIR/$test_dir"
    
    # Step 1: Compile .csm to C from within test directory
    compile_output="$temp_dir/compile_output.txt"
    generated_c="$temp_dir/generated.c"
    
    if ! timeout ${DBG_TEST_TIMEOUT} "$CASM_BIN" --target=c --output="$generated_c" "test.csm" > "$compile_output" 2>&1; then
        # Compilation failed - check if output matches expected
        expected_output=$(cat "output.txt")
        actual_output=$(cat "$compile_output")
        if [ "$expected_output" = "$actual_output" ]; then
            echo "✓ (expected compile error)"
            PASSED=$((PASSED + 1))
        else
            echo "✗ (compile error output mismatch)"
            echo "      Expected:"
            echo "$expected_output" | sed 's/^/        /'
            echo "      Got:"
            echo "$actual_output" | sed 's/^/        /'
            FAILED=$((FAILED + 1))
        fi
        cd "$ORIG_DIR"
        continue
    fi
    
    # Step 2: Validate generated C code matches expected
    if [ -f "out.c" ]; then
        expected_c=$(cat "out.c")
        actual_c=$(cat "$generated_c")
        if [ "$expected_c" != "$actual_c" ]; then
            echo "✗ (C code mismatch)"
            echo "      Expected first 20 lines of out.c:"
            head -20 "out.c" | sed 's/^/        /'
            echo "      Got first 20 lines:"
            head -20 "$generated_c" | sed 's/^/        /'
            FAILED=$((FAILED + 1))
            cd "$ORIG_DIR"
            continue
        fi
    fi
    
    # Step 3: Compile generated C to executable
    executable="$temp_dir/test_exe"
    
    if ! timeout ${DBG_TEST_TIMEOUT} gcc -Wall -Wextra -pedantic -std=c99 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer "$generated_c" -o "$executable" -lm 2>&1 > "$temp_dir/gcc_output.txt"; then
        echo "✗ (C compilation failed)"
        echo "      GCC error output:"
        cat "$temp_dir/gcc_output.txt" | sed 's/^/        /'
        FAILED=$((FAILED + 1))
        cd "$ORIG_DIR"
        continue
    fi
    
    # Step 4: Run the executable and capture output
    if ! timeout ${DBG_TEST_TIMEOUT} "$executable" > "$temp_dir/run_stdout.txt" 2>"$temp_dir/run_stderr.txt"; then
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo "✗ (execution timeout)"
            FAILED=$((FAILED + 1))
            cd "$ORIG_DIR"
            continue
        fi
        # Non-timeout exit codes are allowed (e.g., programs that return non-zero)
    fi
    
    # Step 5: Compare program output (stdout + stderr combined, in that order)
    cat "$temp_dir/run_stdout.txt" "$temp_dir/run_stderr.txt" > "$temp_dir/actual_output.txt"
    expected_output=$(cat "output.txt")
    actual_output=$(cat "$temp_dir/actual_output.txt")
    
    if [ "$expected_output" != "$actual_output" ]; then
        echo "✗ (output mismatch)"
        echo "      Expected output:"
        echo "$expected_output" | sed 's/^/        /'
        echo "      Got output:"
        echo "$actual_output" | sed 's/^/        /'
        FAILED=$((FAILED + 1))
        cd "$ORIG_DIR"
        continue
    fi
    
    # Step 6: Validate WAT code matches expected
    generated_wat="$temp_dir/generated.wat"
    wat_compile_output="$temp_dir/wat_compile_output.txt"
    if ! timeout ${DBG_TEST_TIMEOUT} "$CASM_BIN" --target=wat --output="$generated_wat" "test.csm" > "$wat_compile_output" 2>&1; then
        echo "✗ (WAT compilation failed)"
        echo "      Compilation error:"
        cat "$wat_compile_output" | sed 's/^/        /'
        FAILED=$((FAILED + 1))
        cd "$ORIG_DIR"
        continue
    fi
    
    if [ -f "out.wat" ]; then
        expected_wat=$(cat "out.wat")
        actual_wat=$(cat "$generated_wat")
        if [ "$expected_wat" != "$actual_wat" ]; then
            echo "✗ (WAT code mismatch)"
            echo "      Expected first 20 lines of out.wat:"
            head -20 "out.wat" | sed 's/^/        /'
            echo "      Got first 20 lines:"
            head -20 "$generated_wat" | sed 's/^/        /'
            FAILED=$((FAILED + 1))
            cd "$ORIG_DIR"
            continue
        fi
    fi
    
    # Step 7: WAT execution validation
    wat_executor="$ORIG_DIR/tests/wat_executor.py"
    if [ ! -f "$wat_executor" ]; then
        echo "✗ (WAT executor not found)"
        FAILED=$((FAILED + 1))
        cd "$ORIG_DIR"
        continue
    fi
    
    wat_run_output="$temp_dir/wat_run_output.txt"
    if ! timeout ${DBG_TEST_TIMEOUT} python3 "$wat_executor" "$generated_wat" > "$temp_dir/wat_stdout.txt" 2>"$temp_dir/wat_stderr.txt"; then
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo "✗ (WAT execution timeout)"
            FAILED=$((FAILED + 1))
            cd "$ORIG_DIR"
            continue
        fi
        # Non-timeout exit codes are allowed (e.g., programs that return non-zero)
    fi
    
    # Compare WAT execution output with expected output
    cat "$temp_dir/wat_stdout.txt" "$temp_dir/wat_stderr.txt" > "$temp_dir/wat_actual_output.txt"
    expected_wat_output=$(cat "output.txt")
    actual_wat_output=$(cat "$temp_dir/wat_actual_output.txt")
    
    if [ "$expected_wat_output" != "$actual_wat_output" ]; then
        echo "✗ (WAT output mismatch)"
        echo "      Expected output:"
        echo "$expected_wat_output" | sed 's/^/        /'
        echo "      Got output:"
        echo "$actual_wat_output" | sed 's/^/        /'
        FAILED=$((FAILED + 1))
        cd "$ORIG_DIR"
        continue
    fi
    
    # All checks passed
    echo "✓"
    PASSED=$((PASSED + 1))
    
    # Return to original directory
    cd "$ORIG_DIR"
done

echo ""
echo "DBG Test Results: $PASSED passed, $FAILED failed"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
