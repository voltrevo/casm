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
KNOWN_FAILURES=0

# Save original directory
ORIG_DIR=$(pwd)

if [ ! -f "$ORIG_DIR/bin/casm" ]; then
    echo "✗ bin/casm binary not found. Run 'make' first."
    exit 1
fi

# Python wasmtime module is required for validating WAT code execution
if ! python3 -c "import wasmtime" 2>/dev/null; then
    echo "✗ wasmtime Python module is required but not found."
    echo "  Install with: pip3 install wasmtime"
    exit 1
fi

# Find all dbg test directories (recursively find directories containing test.csm)
while IFS= read -r test_file; do
    test_dir=$(dirname "$test_file")
    test_name=$(basename "$test_dir")
    test_file="${test_dir%/}/test.csm"
    output_file="${test_dir%/}/output.txt"
    expected_c_file="${test_dir%/}/out.c"
    expected_wat_file="${test_dir%/}/out.wat"
    known_failure_file="${test_dir%/}/known_failure.txt"
    
    is_known_failure=false
    if [ -f "$known_failure_file" ]; then
        is_known_failure=true
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
        # Compilation failed
        expected_output=$(cat "output.txt")
        actual_output=$(cat "$compile_output")
        if [ "$expected_output" = "$actual_output" ]; then
            if [ "$is_known_failure" = true ]; then
                # Known failure case: failure is expected - mark as known failure (suppress output)
                echo "✓ (known failure)"
                KNOWN_FAILURES=$((KNOWN_FAILURES + 1))
            else
                # Normal case: this is a compile error test
                echo "✓"
                PASSED=$((PASSED + 1))
            fi
        else
            if [ "$is_known_failure" = true ]; then
                # Known failure case: should fail, and it did, but output doesn't match - suppress this too
                echo "✓ (known failure)"
                KNOWN_FAILURES=$((KNOWN_FAILURES + 1))
            else
                echo "✗ (compile error output mismatch)"
                echo "      Expected:"
                echo "$expected_output" | sed 's/^/        /'
                echo "      Got:"
                echo "$actual_output" | sed 's/^/        /'
                FAILED=$((FAILED + 1))
            fi
        fi
        cd "$ORIG_DIR"
        continue
    fi
    
    # If this is a known failure, we expected failure but got success - that's a failure
    if [ "$is_known_failure" = true ]; then
        echo "✗ (passing test marked as known failure)"
        FAILED=$((FAILED + 1))
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
done < <(find tests/dbg_cases -name "test.csm" -type f)

echo ""
echo "DBG Test Results: $PASSED passed, $FAILED failed, $KNOWN_FAILURES known failures"

# Collect and report coverage
echo ""
echo "=========================================="
echo "Collecting Coverage Data"
echo "=========================================="

COVERAGE_DIR="$ORIG_DIR/coverage_report"
COVERAGE_INFO="$COVERAGE_DIR/coverage.info"
rm -rf "$COVERAGE_DIR"
mkdir -p "$COVERAGE_DIR"

# Collect coverage data from .gcda files
echo "Collecting branch coverage metrics..."
if lcov --capture \
  --directory "$ORIG_DIR/bin" \
  --output-file "$COVERAGE_INFO" \
  --branch-coverage \
  2>/dev/null; then
  
  # Extract overall statistics from coverage.info
  lines_hit=$(grep "^LH:" "$COVERAGE_INFO" | awk -F: '{s+=$2} END {print s}')
  lines_total=$(grep "^LF:" "$COVERAGE_INFO" | awk -F: '{s+=$2} END {print s}')
  branches_hit=$(grep "^BRH:" "$COVERAGE_INFO" | awk -F: '{s+=$2} END {print s}')
  branches_total=$(grep "^BRF:" "$COVERAGE_INFO" | awk -F: '{s+=$2} END {print s}')
  functions_hit=$(grep "^FNH:" "$COVERAGE_INFO" | awk -F: '{s+=$2} END {print s}')
  functions_total=$(grep "^FNF:" "$COVERAGE_INFO" | awk -F: '{s+=$2} END {print s}')
  
  # Calculate percentages
  calc_percent() {
    local hit=$1
    local total=$2
    if [ "$total" -eq 0 ] || [ -z "$total" ]; then
      echo "0.0"
    else
      awk "BEGIN {printf \"%.1f\", ($hit * 100.0) / $total}"
    fi
  }
  
  branch_pct=$(calc_percent "$branches_hit" "$branches_total")
  line_pct=$(calc_percent "$lines_hit" "$lines_total")
  function_pct=$(calc_percent "$functions_hit" "$functions_total")
  
  echo ""
  echo "=========================================="
  echo "       COVERAGE REPORT - BRANCH COVERAGE"
  echo "=========================================="
  echo ""
  echo "Branch Coverage:     $branch_pct%  ($branches_hit / $branches_total branches)"
  echo "Line Coverage:       $line_pct%   ($lines_hit / $lines_total lines)"
  echo "Function Coverage:   $function_pct%  ($functions_hit / $functions_total functions)"
  echo ""
  echo "HTML Report:   $COVERAGE_DIR/index.html"
  echo "=========================================="
  echo ""
  
  # Generate HTML report
  genhtml "$COVERAGE_INFO" \
    --output-directory "$COVERAGE_DIR" \
    --branch-coverage \
    --highlight \
    --legend \
    --title "Casm Compiler - Branch Coverage Report (DBG Tests)" \
    2>/dev/null || true
else
  echo "Warning: Could not collect coverage data"
fi

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
