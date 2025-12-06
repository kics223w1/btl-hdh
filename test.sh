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
    "os_syscall"
    "os_syscall_list"
    "os_1_mlq_paging_small_1K"
    "os_1_mlq_paging_small_4K"
    "os_sc"
)

# Note:
# - os_1_singleCPU_mlq: Skipped (Legacy format, missing memory config line)
# - sched_*: Skipped (For 'sched' target, not 'os' target)

# Choose a timeout command if available (Linux: timeout, macOS/Homebrew: gtimeout)
TIMEOUT_CMD=""
if command -v timeout >/dev/null 2>&1; then
    TIMEOUT_CMD="timeout 10"
elif command -v gtimeout >/dev/null 2>&1; then
    TIMEOUT_CMD="gtimeout 10"
fi

for test in "${TEST_FILES[@]}"; do
    TOTAL=$((TOTAL+1))
    echo "[Test] Running $test..."

    if [ -n "$TIMEOUT_CMD" ]; then
        # Run with timeout protection if available
        if $TIMEOUT_CMD ./os "$test" > /dev/null 2>&1; then
            echo -e "${GREEN}✅ PASS${NC} - $test"
            PASSED=$((PASSED+1))
        else
            echo -e "${RED}❌ FAIL${NC} - $test"
        fi
    else
        # Fallback: run directly (no timeout available on this platform)
        if ./os "$test" > /dev/null 2>&1; then
            echo -e "${GREEN}✅ PASS${NC} - $test"
            PASSED=$((PASSED+1))
        else
            echo -e "${RED}❌ FAIL${NC} - $test"
        fi
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

