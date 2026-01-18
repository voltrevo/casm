#!/bin/bash

# Tokenization test script

COMPILER="./casm"
EXAMPLES_DIR="examples"
PASS=0
FAIL=0

run_test() {
    local name=$1
    local file=$2
    local should_pass=$3
    
    echo -n "Testing $name... "
    
    output=$($COMPILER "$file" 2>&1)
    exit_code=$?
    
    if [ "$should_pass" = "true" ]; then
        if [ $exit_code -eq 0 ] && echo "$output" | grep -q "EOF"; then
            echo "PASS"
            ((PASS++))
        else
            echo "FAIL (expected to pass)"
            echo "  Output: $output"
            ((FAIL++))
        fi
    else
        if [ $exit_code -ne 0 ]; then
            echo "PASS (correctly rejected)"
            ((PASS++))
        else
            echo "FAIL (expected to fail)"
            echo "  Output: $output"
            ((FAIL++))
        fi
    fi
}

echo "Running tokenization tests..."
echo

# Valid programs
run_test "simple_add" "$EXAMPLES_DIR/simple_add.csm" "true"
run_test "variables" "$EXAMPLES_DIR/variables.csm" "true"
run_test "function_call" "$EXAMPLES_DIR/function_call.csm" "true"
run_test "if_statement" "$EXAMPLES_DIR/if_statement.csm" "true"
run_test "while_loop" "$EXAMPLES_DIR/while_loop.csm" "true"
run_test "for_loop" "$EXAMPLES_DIR/for_loop.csm" "true"
run_test "all_operators" "$EXAMPLES_DIR/all_operators.csm" "true"
run_test "bool_type" "$EXAMPLES_DIR/bool_type.csm" "true"
run_test "mixed_types" "$EXAMPLES_DIR/mixed_types.csm" "true"

echo
echo "========================================="
echo "Tests passed: $PASS"
echo "Tests failed: $FAIL"
echo "========================================="

if [ $FAIL -eq 0 ]; then
    exit 0
else
    exit 1
fi
