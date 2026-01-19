#!/bin/bash
# Test runner with aggressive timeout protection
# Assumes binaries are already built; just runs them

set -e

UNIT_TEST_TIMEOUT=1
EXAMPLE_TEST_TIMEOUT=1

echo "=========================================="
echo "Running Casm Compiler Tests"
echo "=========================================="

# Check that test binaries exist
if [ ! -f "./bin/test_casm" ]; then
    echo "✗ bin/test_casm binary not found. Run 'make build-debug' first."
    exit 1
fi

if [ ! -f "./bin/test_semantics" ]; then
    echo "✗ bin/test_semantics binary not found. Run 'make build-debug' first."
    exit 1
fi

if [ ! -f "./bin/test_codegen" ]; then
    echo "✗ bin/test_codegen binary not found. Run 'make build-debug' first."
    exit 1
fi

if [ ! -f "./bin/casm" ]; then
    echo "✗ bin/casm binary not found. Run 'make build-debug' first."
    exit 1
fi

# Run unit tests with timeout
echo ""
echo "Running unit tests (timeout: ${UNIT_TEST_TIMEOUT}s)..."
unit_output=$(mktemp)
if timeout ${UNIT_TEST_TIMEOUT} ./bin/test_casm >"$unit_output" 2>&1; then
    echo "✓ Unit tests passed"
    cat "$unit_output"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "✗ Unit tests timed out after ${UNIT_TEST_TIMEOUT}s"
        exit 1
    else
        echo "✗ Unit tests failed"
        echo "Error output:"
        cat "$unit_output"
        exit 1
    fi
fi
rm -f "$unit_output"

# Run semantics tests with timeout
echo ""
echo "Running semantics tests (timeout: ${UNIT_TEST_TIMEOUT}s)..."
semantics_output=$(mktemp)
if timeout ${UNIT_TEST_TIMEOUT} ./bin/test_semantics >"$semantics_output" 2>&1; then
    echo "✓ Semantics tests passed"
    cat "$semantics_output"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "✗ Semantics tests timed out after ${UNIT_TEST_TIMEOUT}s"
        exit 1
    else
        echo "✗ Semantics tests failed"
        echo "Error output:"
        cat "$semantics_output"
        exit 1
    fi
fi
rm -f "$semantics_output"

# Run codegen tests with timeout
echo ""
echo "Running codegen tests (timeout: ${UNIT_TEST_TIMEOUT}s)..."
codegen_output=$(mktemp)
if timeout ${UNIT_TEST_TIMEOUT} ./bin/test_codegen >"$codegen_output" 2>&1; then
    echo "✓ Codegen tests passed"
    cat "$codegen_output"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "✗ Codegen tests timed out after ${UNIT_TEST_TIMEOUT}s"
        exit 1
    else
        echo "✗ Codegen tests failed"
        echo "Error output:"
        cat "$codegen_output"
        exit 1
    fi
fi
rm -f "$codegen_output"

# Test supported examples (those without unsupported control flow)
echo ""
echo "Running example tests (timeout: ${EXAMPLE_TEST_TIMEOUT}s per file)..."
EXAMPLES_PASSED=0
EXAMPLES_FAILED=0

# These examples use only supported features (functions, variables, expressions, returns)
SUPPORTED_EXAMPLES=(
    "examples/simple_add.csm"
    "examples/variables.csm"
    "examples/function_call.csm"
    "examples/mixed_types.csm"
    "examples/if_statement.csm"
    "examples/while_loop.csm"
    "examples/for_loop.csm"
)

for example in "${SUPPORTED_EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        echo -n "  Testing $example... "
        error_output=$(mktemp)
        if timeout ${EXAMPLE_TEST_TIMEOUT} ./bin/casm --target=c "$example" >"$error_output" 2>&1; then
            echo "✓"
            EXAMPLES_PASSED=$((EXAMPLES_PASSED + 1))
            rm -f "$error_output"
        else
            EXIT_CODE=$?
            if [ $EXIT_CODE -eq 124 ]; then
                echo "✗ (timeout)"
            else
                echo "✗ (failed)"
                echo "    Error output:"
                sed 's/^/    /' "$error_output"
            fi
            EXAMPLES_FAILED=$((EXAMPLES_FAILED + 1))
            rm -f "$error_output"
        fi
    fi
done

# Run dbg tests with timeout
echo ""
echo "Running dbg tests (timeout: 2s per test)..."
echo "Cleaning coverage data before DBG tests..."
find ./bin -name "*.gcda" -delete 2>/dev/null || true
if timeout 30 tests/run_dbg_tests.sh; then
    DBG_TEST_RESULT="PASSED"
    echo "✓ DBG tests passed"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "✗ DBG tests timed out"
        DBG_TEST_RESULT="FAILED"
    else
        echo "✗ DBG tests failed"
        DBG_TEST_RESULT="FAILED"
    fi
fi

echo ""
echo "=========================================="
echo "Test Results:"
echo "  Unit tests: PASSED"
echo "  Examples: $EXAMPLES_PASSED passed, $EXAMPLES_FAILED failed"
echo "  DBG tests: $DBG_TEST_RESULT"
echo "=========================================="

if [ $EXAMPLES_FAILED -gt 0 ] || [ "$DBG_TEST_RESULT" = "FAILED" ]; then
    exit 1
fi

exit 0

