#!/bin/bash

# Test Runner Script for Simple Operating System
# This script runs all input tests, saves results to result/, and compares with expected output/

echo "====================================="
echo "  OS Test Runner & Comparison"
echo "====================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories
INPUT_DIR="input"
OUTPUT_DIR="output"
RESULT_DIR="result"

# Test counters
TOTAL=0
PASSED=0
FAILED=0

# Create result directory if it doesn't exist
mkdir -p "$RESULT_DIR"

# Build the project first
echo "[Build] Compiling project..."
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    echo -e "${GREEN}[OK]${NC} Build successful"
else
    echo -e "${RED}[FAIL]${NC} Build failed"
    exit 1
fi
echo ""

# Choose a timeout command if available (Linux: timeout, macOS/Homebrew: gtimeout)
TIMEOUT_CMD=""
if command -v timeout >/dev/null 2>&1; then
    TIMEOUT_CMD="timeout 30"
elif command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_CMD="gtimeout 30"
fi

# Array to store failed tests for summary
declare -a FAILED_TESTS=()

echo "====================================="
echo "  Running Tests"
echo "====================================="
echo ""

# Loop through all expected output files
for expected_file in "$OUTPUT_DIR"/*.output; do
    # Extract test name from filename
    filename=$(basename "$expected_file")
    test_name="${filename%.output}"
    
    # Check if corresponding input file exists
    input_file="$INPUT_DIR/$test_name"
    if [ ! -f "$input_file" ]; then
        echo -e "${YELLOW}[SKIP]${NC} $test_name - Input file not found"
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    result_file="$RESULT_DIR/${test_name}.output"
    
    echo -n "[$TOTAL] Testing $test_name... "
    
    # Run the test with timeout if available
    if [ -n "$TIMEOUT_CMD" ]; then
        $TIMEOUT_CMD ./os "$test_name" > "$result_file" 2>&1
        exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo -e "${RED}[TIMEOUT]${NC}"
            FAILED=$((FAILED + 1))
            FAILED_TESTS+=("$test_name (timeout)")
            continue
        fi
    else
        ./os "$test_name" > "$result_file" 2>&1
    fi
    
    # Compare output with expected
    if diff -q "$expected_file" "$result_file" > /dev/null 2>&1; then
        echo -e "${GREEN}[PASS]${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC}"
        FAILED=$((FAILED + 1))
        FAILED_TESTS+=("$test_name")
    fi
done

echo ""
echo "====================================="
echo "  Test Summary"
echo "====================================="
echo "Total:  $TOTAL"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

# Show details for failed tests
if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo "====================================="
    echo "  Failed Test Details"
    echo "====================================="
    echo ""
    
    for test_name in "${FAILED_TESTS[@]}"; do
        # Skip timeout tests for diff display
        if [[ "$test_name" == *"(timeout)"* ]]; then
            echo "--- $test_name ---"
            echo "Test timed out after 30 seconds"
            echo ""
            continue
        fi
        
        echo "--- $test_name ---"
        echo "Diff between expected (output/) and actual (result/):"
        diff "$OUTPUT_DIR/${test_name}.output" "$RESULT_DIR/${test_name}.output" | head -50
        echo ""
    done
fi

# Final result
echo "====================================="
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Check the diffs above.${NC}"
    echo ""
    echo "To view full diff for a specific test:"
    echo "  diff output/<test>.output result/<test>.output"
    exit 1
fi
