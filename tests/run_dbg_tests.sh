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
XFAILED=0

# Save original directory
ORIG_DIR=$(pwd)

if [ ! -f "$ORIG_DIR/bin/casm" ]; then
    echo "✗ bin/casm binary not found. Run 'make' first."
    exit 1
fi

# Find all dbg test directories
for test_dir in tests/dbg_cases/*/; do
    test_name=$(basename "$test_dir")
    test_file="${test_dir%/}/test.csm"
    output_file="${test_dir%/}/output.txt"
    expected_c_file="${test_dir%/}/out.c"
    expected_wat_file="${test_dir%/}/out.wat"
    known_failure_file="${test_dir%/}/known_failure.txt"
    known_failure=0
    if [ -f "$known_failure_file" ]; then
        known_failure=1
    fi
    
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
    test_failed=0
    fail_reason=""
    compile_handled=0
    
    if ! timeout ${DBG_TEST_TIMEOUT} "$CASM_BIN" --target=c --output="$generated_c" "test.csm" > "$compile_output" 2>&1; then
        # Compilation failed - check if output matches expected
        expected_output=$(cat "output.txt")
        actual_output=$(cat "$compile_output")
        if [ "$expected_output" = "$actual_output" ]; then
            if [ $known_failure -eq 1 ]; then
                test_failed=1
                fail_reason="expected compile error"
            fi
        else
            test_failed=1
            fail_reason="compile error output mismatch"
        fi
        compile_handled=1
    fi
    
    # Step 2: Validate generated C code matches expected
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ] && [ -f "out.c" ]; then
        expected_c=$(cat "out.c")
        actual_c=$(cat "$generated_c")
        if [ "$expected_c" != "$actual_c" ]; then
            test_failed=1
            fail_reason="C code mismatch"
        fi
    fi
    
    # Step 3: Compile generated C to executable
    executable="$temp_dir/test_exe"
    
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ]; then
        if ! timeout ${DBG_TEST_TIMEOUT} gcc -Wall -Wextra -pedantic -std=c99 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer "$generated_c" -o "$executable" -lm 2>&1 > "$temp_dir/gcc_output.txt"; then
            test_failed=1
            fail_reason="C compilation failed"
        fi
    fi
    
    # Step 4: Run the executable and capture output
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ]; then
        if ! timeout ${DBG_TEST_TIMEOUT} "$executable" > "$temp_dir/run_stdout.txt" 2>"$temp_dir/run_stderr.txt"; then
            EXIT_CODE=$?
            if [ $EXIT_CODE -eq 124 ]; then
                test_failed=1
                fail_reason="execution timeout"
            fi
            # Non-timeout exit codes are allowed (e.g., programs that return non-zero)
        fi
    fi
    
    # Step 5: Compare program output (stdout + stderr combined, in that order)
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ]; then
        cat "$temp_dir/run_stdout.txt" "$temp_dir/run_stderr.txt" > "$temp_dir/actual_output.txt"
        expected_output=$(cat "output.txt")
        actual_output=$(cat "$temp_dir/actual_output.txt")
        
        if [ "$expected_output" != "$actual_output" ]; then
            test_failed=1
            fail_reason="output mismatch"
        fi
    fi
    
    # Step 6: Validate WAT code matches expected
    generated_wat="$temp_dir/generated.wat"
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ]; then
        if ! timeout ${DBG_TEST_TIMEOUT} "$CASM_BIN" --target=wat --output="$generated_wat" "test.csm" > /dev/null 2>&1; then
            test_failed=1
            fail_reason="WAT compilation failed"
        fi
    fi
    
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ] && [ -f "out.wat" ]; then
        expected_wat=$(cat "out.wat")
        actual_wat=$(cat "$generated_wat")
        if [ "$expected_wat" != "$actual_wat" ]; then
            test_failed=1
            fail_reason="WAT code mismatch"
        fi
    fi
    
    # Step 7: Execute WAT and verify it doesn't crash
    if [ $compile_handled -eq 0 ] && [ $test_failed -eq 0 ]; then
        if command -v wasmtime &> /dev/null; then
            if ! timeout ${DBG_TEST_TIMEOUT} wasmtime "$generated_wat" --invoke main > /dev/null 2>&1; then
                test_failed=1
                fail_reason="WAT execution failed"
            fi
        fi
    fi
    
    # All checks passed
    if [ $known_failure -eq 1 ]; then
        if [ $test_failed -eq 0 ]; then
            echo "✗ (known failure passed)"
            FAILED=$((FAILED + 1))
        else
            if [ -z "$fail_reason" ]; then
                fail_reason="failed as expected"
            fi
            echo "⚠️ (known failure: $fail_reason)"
            XFAILED=$((XFAILED + 1))
        fi
    else
        if [ $test_failed -eq 0 ]; then
            if [ $compile_handled -eq 1 ]; then
                echo "✓ (expected compile error)"
            else
                echo "✓"
            fi
            PASSED=$((PASSED + 1))
        else
            echo "✗ ($fail_reason)"
            FAILED=$((FAILED + 1))
        fi
    fi
    
    # Return to original directory
    cd "$ORIG_DIR"
done

echo ""
echo "DBG Test Results: $PASSED passed, $FAILED failed, $XFAILED known failures"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
