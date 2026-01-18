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

if [ ! -f "./bin/casm" ]; then
    echo "✗ bin/casm binary not found. Run 'make build-debug' first."
    exit 1
fi

# Run unit tests with timeout
echo ""
echo "Running unit tests (timeout: ${UNIT_TEST_TIMEOUT}s)..."
if timeout ${UNIT_TEST_TIMEOUT} ./bin/test_casm; then
    echo "✓ Unit tests passed"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "✗ Unit tests timed out after ${UNIT_TEST_TIMEOUT}s"
        exit 1
    else
        echo "✗ Unit tests failed"
        exit 1
    fi
fi

# Run semantics tests with timeout
echo ""
echo "Running semantics tests (timeout: ${UNIT_TEST_TIMEOUT}s)..."
if timeout ${UNIT_TEST_TIMEOUT} ./bin/test_semantics; then
    echo "✓ Semantics tests passed"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 124 ]; then
        echo "✗ Semantics tests timed out after ${UNIT_TEST_TIMEOUT}s"
        exit 1
    else
        echo "✗ Semantics tests failed"
        exit 1
    fi
fi

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
)

for example in "${SUPPORTED_EXAMPLES[@]}"; do
    if [ -f "$example" ]; then
        echo -n "  Testing $example... "
        if timeout ${EXAMPLE_TEST_TIMEOUT} ./bin/casm "$example" >/dev/null 2>&1; then
            echo "✓"
            EXAMPLES_PASSED=$((EXAMPLES_PASSED + 1))
        else
            EXIT_CODE=$?
            if [ $EXIT_CODE -eq 124 ]; then
                echo "✗ (timeout)"
            else
                echo "✗ (failed)"
            fi
            EXAMPLES_FAILED=$((EXAMPLES_FAILED + 1))
        fi
    fi
done

echo ""
echo "=========================================="
echo "Test Results:"
echo "  Unit tests: PASSED"
echo "  Examples: $EXAMPLES_PASSED passed, $EXAMPLES_FAILED failed"
echo "=========================================="

if [ $EXAMPLES_FAILED -gt 0 ]; then
    exit 1
fi

exit 0

