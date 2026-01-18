#!/bin/bash
# DBG test runner - tests the dbg() debugging feature
# Each test case is a directory containing:
#   test.csm - the source file to compile
#   output.txt - expected combined stdout+stderr output
#   (optional) compile_error - if present, test expects compilation to fail

set -e

DBG_TEST_TIMEOUT=2
CASM_BIN="./bin/casm"
PASSED=0
FAILED=0

if [ ! -f "$CASM_BIN" ]; then
    echo "✗ $CASM_BIN binary not found. Run 'make' first."
    exit 1
fi

# Find all dbg test directories
for test_dir in tests/dbg_cases/*/; do
    test_name=$(basename "$test_dir")
    test_file="${test_dir%/}/test.csm"  # Remove trailing slash then add test.csm
    output_file="${test_dir%/}/output.txt"
    compile_error_marker="${test_dir%/}/compile_error"
    
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
    
    # Normalize the test file path (relative from cwd)
    # Remove leading ./ and resolve to canonical form
    normalized_test_file=$(echo "$test_file" | sed 's|^\./||')
    
    # Step 1: Compile .csm to C
    compile_output="$temp_dir/compile_output.txt"
    generated_c="$temp_dir/generated.c"
    
    if ! timeout ${DBG_TEST_TIMEOUT} "$CASM_BIN" --target=c --output="$generated_c" "$normalized_test_file" > "$compile_output" 2>&1; then
        if [ -f "$compile_error_marker" ]; then
            # Expected compilation error - check if error output matches expected
            expected_output=$(cat "$output_file")
            actual_output=$(cat "$compile_output")
            if [ "$expected_output" = "$actual_output" ]; then
                echo "✓ (expected compile error)"
                PASSED=$((PASSED + 1))
            else
                echo "✗ (compile error output mismatch)"
                FAILED=$((FAILED + 1))
            fi
        else
            # Unexpected compilation error
            echo "✗ (compilation failed)"
            FAILED=$((FAILED + 1))
        fi
        continue
    fi
    
    # If compilation succeeded but error was expected, that's a failure
    if [ -f "$compile_error_marker" ]; then
        echo "✗ (expected compile error but succeeded)"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Step 2: Compile generated C to executable
    executable="$temp_dir/test_exe"
    
    if ! timeout ${DBG_TEST_TIMEOUT} gcc -Wall -Wextra -pedantic -std=c99 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer "$generated_c" -o "$executable" -lm 2>&1 > "$temp_dir/gcc_output.txt"; then
        echo "✗ (C compilation failed)"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Step 3: Run the executable and capture output
    if ! timeout ${DBG_TEST_TIMEOUT} "$executable" > "$temp_dir/run_stdout.txt" 2>"$temp_dir/run_stderr.txt"; then
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo "✗ (execution timeout)"
            FAILED=$((FAILED + 1))
            continue
        fi
        # Non-timeout exit codes are allowed (e.g., programs that return non-zero)
    fi
    
    # Step 4: Compare output (stdout + stderr combined, in that order)
    cat "$temp_dir/run_stdout.txt" "$temp_dir/run_stderr.txt" > "$temp_dir/actual_output.txt"
    expected_output=$(cat "$output_file")
    actual_output=$(cat "$temp_dir/actual_output.txt")
    
    if [ "$expected_output" = "$actual_output" ]; then
        echo "✓"
        PASSED=$((PASSED + 1))
    else
        echo "✗ (output mismatch)"
        echo "    Expected:"
        echo "$expected_output" | sed 's/^/      /'
        echo "    Actual:"
        echo "$actual_output" | sed 's/^/      /'
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "DBG Test Results: $PASSED passed, $FAILED failed"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
