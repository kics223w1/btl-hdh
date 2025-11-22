#!/bin/bash

# Test script for Simple Operating System
# Usage: ./test.sh

echo "====================================="
echo "  Simple OS - Test Script"
echo "====================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test counter
TOTAL=0
PASSED=0

# Build test
echo "[1/8] Building project..."
TOTAL=$((TOTAL+1))
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    echo -e "${GREEN}✅ PASS${NC} - Build successful"
    PASSED=$((PASSED+1))
else
    echo -e "${RED}❌ FAIL${NC} - Build failed"
    exit 1
fi
echo ""

# Test scenarios
TEST_FILES=(
    "os_1_mlq_paging"
    "os_0_mlq_paging"
    "os_1_singleCPU_mlq_paging"
)

for test in "${TEST_FILES[@]}"; do
    TOTAL=$((TOTAL+1))
    echo "[Test] Running $test..."
    
    if timeout 10 ./os "$test" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ PASS${NC} - $test"
        PASSED=$((PASSED+1))
    else
        echo -e "${RED}❌ FAIL${NC} - $test (may timeout on macOS without gtimeout)"
    fi
done

echo ""
echo "====================================="
echo "  Test Summary"
echo "====================================="
echo "Total:  $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $((TOTAL-PASSED))"
echo ""

if [ $PASSED -eq $TOTAL ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}⚠️  Some tests failed${NC}"
    exit 1
fi

