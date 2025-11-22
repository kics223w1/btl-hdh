# How to Run and Test - Simple OS

**Current Configuration**: 32-bit mode (default)
**Input Format**: Custom memory sizes (MM_FIXED_MEMSZ disabled)

## Build

```bash
cd /Applications/dev/btl-hdh
make clean && make all
```

Expected: Compiles successfully (1 warning in mem.c is pre-existing, not an error)

## Run

### Basic Execution

```bash
./os <config_file>
```

### Test Scenarios

```bash
# Test 1: Default (4 CPUs, MLQ, Paging)
./os os_1_mlq_paging

# Test 2: Single CPU
./os os_1_singleCPU_mlq_paging

# Test 3: No paging overhead
./os os_0_mlq_paging

# Test 4: Small memory (1KB RAM)
./os os_1_mlq_paging_small_1K

# Test 5: Small memory (4KB RAM)
./os os_1_mlq_paging_small_4K

# Test 6: Single CPU with paging
./os os_1_singleCPU_mlq_paging
```

## Compare Output

```bash
# Run test
./os os_1_mlq_paging > my_output.txt

# Compare with expected (may differ due to concurrency)
diff my_output.txt output/os_1_mlq_paging.output
```

Note: Outputs may not match exactly due to concurrent execution, but should be logically equivalent.

## Configuration

### Change Settings

```bash
# 1. Edit configuration
nano include/os-cfg.h

# 2. Rebuild
make clean && make all
```

### Quick Switches

**Enable 64-bit mode**:
```c
// include/os-cfg.h
#define MM64 1
//#undef MM64
```

**Use fixed memory (backward compatible)**:
```c
// include/os-cfg.h
#define MM_FIXED_MEMSZ
```

**Enable debug output**:
```c
// include/os-cfg.h
#define VMDBG 1
#define MMDBG 1
```

## Create Custom Test

### Input File Format

**With MM_FIXED_MEMSZ disabled** (default):
```
<time_slice> <num_cpus> <num_processes>
<RAM_SIZE> <SWAP0> <SWAP1> <SWAP2> <SWAP3>
<time> <process_path> <priority>
...
```

**With MM_FIXED_MEMSZ enabled**:
```
<time_slice> <num_cpus> <num_processes>
<time> <process_path> <priority>
...
```

### Example

Create `input/my_test`:
```
2 2 4
1048576 16777216 0 0 0
1 p0s 10
2 s1 20
3 m0s 30
4 s2 40
```

Run:
```bash
./os my_test
```

## Verify Implementation

### Check Build

```bash
make clean && make all 2>&1 | grep -i error
```

Expected: No output (no errors)

### Check Functions

```bash
# Scheduler functions
nm obj/queue.o | grep -E "enqueue|dequeue"
nm obj/sched.o | grep -E "get_mlq_proc"

# Memory functions
nm obj/mm-memphy.o | grep -E "MEMPHY"
nm obj/mm64.o | grep -E "init_mm|pte_set"
nm obj/libmem.o | grep -E "pg_get"
```

Expected: All functions present

### Run All Tests

```bash
for test in os_0_mlq_paging os_1_mlq_paging os_1_singleCPU_mlq_paging; do
    echo "Testing $test..."
    ./os $test > /dev/null 2>&1 && echo "✅ PASS" || echo "❌ FAIL"
done
```

## Debug

### Enable Debug Output

```bash
# Edit include/os-cfg.h
# Uncomment: #define VMDBG 1
# Uncomment: #define MMDBG 1

make clean && make all
./os os_1_mlq_paging 2>&1 | tee debug.log
```

### Check Memory Statistics

The OS automatically dumps memory information if `IODUMP` is enabled:
- Memory allocations with addresses
- Page table entries
- Frame mappings

### View Process Scheduling

Output shows:
```
Time slot X
    CPU Y: Dispatched process Z
    CPU Y: Put process Z to run queue
```

Higher priority processes appear first and get more time slots.

## Troubleshooting

### Build Errors

```bash
# Check configuration
cat include/os-cfg.h | grep "^#define"

# Clean rebuild
rm -rf obj
make all
```

### Runtime Errors

**"Cannot find configure file"**:
- Check file exists: `ls input/<filename>`
- Use correct path: `./os <filename>` (without "input/" prefix)

**Segmentation fault**:
- Check `MM_FIXED_MEMSZ` matches input file format
- Verify memory sizes are reasonable

**Wrong output**:
- Concurrent execution causes variations
- Check priority ordering is correct
- Verify processes load and complete

## Performance Testing

```bash
# Measure execution time
time ./os os_1_mlq_paging > /dev/null

# Check memory usage
/usr/bin/time -l ./os os_1_mlq_paging 2>&1 | grep "maximum resident"
```

## Summary

**Build**: `make all`  
**Run**: `./os <config_file>`  
**Test**: Compare with `output/` directory  
**Configure**: Edit `include/os-cfg.h`  
**Debug**: Enable VMDBG, MMDBG flags  

**Status**: ✅ Complete and tested

---

For detailed documentation, see:
- `IMPLEMENTATION_INDEX.md` - Documentation guide
- `SCHEDULER_SUMMARY.md` - Scheduler reference
- `CONFIGURATION_GUIDE.md` - Configuration details

