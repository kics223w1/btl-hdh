#!/bin/bash

# Test Runner Script for os64
# This script runs all input tests and saves results to result/

echo "====================================="
echo "  os64 Test Runner"
echo "====================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories
INPUT_DIR="input"
RESULT_DIR="result"

# Create result directory if it doesn't exist
mkdir -p "$RESULT_DIR"

# Build the project first
echo "[Build] Cleaning and compiling os64..."
if make clean > /dev/null 2>&1 && make os64 > /dev/null 2>&1; then
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

echo "====================================="
echo "  Running Tests"
echo "====================================="
echo ""

# Loop through all files in INPUT_DIR
for input_file in "$INPUT_DIR"/*; do
    # Skip if not a file
    if [ ! -f "$input_file" ]; then
        continue
    fi
    
    filename=$(basename "$input_file")
    test_name="$filename"
    
    echo -n "Running $test_name... "
    result_file="$RESULT_DIR/${test_name}.output"
    
    # Run the test
    if [ -n "$TIMEOUT_CMD" ]; then
        $TIMEOUT_CMD ./os64 "$test_name" > "$result_file" 2>&1
        exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo -e "${RED}[TIMEOUT]${NC}"
            continue
        fi
    else
        ./os64 "$test_name" > "$result_file" 2>&1
    fi
    
    echo -e "${GREEN}[DONE]${NC}"
done

echo ""
echo "====================================="
echo "  All tests executed. Results in $RESULT_DIR/"
echo "====================================="
