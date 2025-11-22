# Configuration Guide - OS Memory Management

## Overview

This guide explains all configuration options available in the simple OS, focusing on the memory management system. The configuration system uses C preprocessor directives (`#define`) to enable/disable features and maintain backward compatibility.

## Configuration File

**Location**: `include/os-cfg.h`

This file controls all system-wide configuration options.

## Configuration Options

### 1. MLQ_SCHED - Multi-Level Queue Scheduler

```c
#define MLQ_SCHED 1
#define MAX_PRIO 140
```

**Purpose**: Enable multi-level queue scheduling with priority levels

**Effect**:
- Enables priority-based scheduling
- Adds `prio` field to PCB structure
- Supports 140 priority levels (similar to Linux)

**When to use**:
- ✅ For realistic scheduling simulation
- ✅ When testing priority-based algorithms
- ❌ Disable for simple FIFO scheduling

### 2. MM_PAGING - Paging Memory Management

```c
#define MM_PAGING
```

**Purpose**: Enable paging-based memory management system

**Effect**:
- Adds memory management fields to kernel and PCB
- Enables virtual memory with paging
- Activates page table management
- Enables RAM and SWAP devices

**PCB Structure Changes**:
```c
struct krnl_t {
#ifdef MM_PAGING
    struct mm_struct *mm;              // Memory manager
    struct memphy_struct *mram;        // RAM device
    struct memphy_struct **mswp;       // SWAP devices
    struct memphy_struct *active_mswp; // Active SWAP
    uint32_t active_mswp_id;          // Active SWAP ID
#endif
};
```

**When to use**:
- ✅ Always (required for virtual memory)
- ❌ Never disable (core feature)

### 3. MM_FIXED_MEMSZ - Fixed Memory Size (Backward Compatibility)

```c
//#define MM_FIXED_MEMSZ    // Commented out = disabled
```

**Purpose**: Backward compatibility with old input file format

**Effect When ENABLED** (`#define MM_FIXED_MEMSZ`):
- Uses fixed memory sizes:
  - RAM: 1 MB (0x100000 = 1048576 bytes)
  - SWAP[0]: 16 MB (0x1000000 = 16777216 bytes)
  - SWAP[1-3]: 0 (disabled)
- Input file has **2 lines** before process list
- Old format compatible

**Effect When DISABLED** (default):
- Reads memory sizes from input file
- Input file has **3 lines** before process list
- New format with custom memory sizes

**Input File Format**:

**WITH MM_FIXED_MEMSZ** (Old Format - 2 lines):
```
[time_slice] [num_cpus] [num_processes]
[time0] [path0] [priority0]
[time1] [path1] [priority1]
...
```

**WITHOUT MM_FIXED_MEMSZ** (New Format - 3 lines):
```
[time_slice] [num_cpus] [num_processes]
[RAM_SIZE] [SWAP0_SIZE] [SWAP1_SIZE] [SWAP2_SIZE] [SWAP3_SIZE]
[time0] [path0] [priority0]
[time1] [path1] [priority1]
...
```

**When to use**:
- ✅ Enable for old input files (2 lines header)
- ❌ Disable for custom memory configurations (3 lines header)

### 4. MM64 - 64-bit Memory Addressing

```c
//#define MM64 1
#undef MM64          // Currently 32-bit mode
```

**Purpose**: Switch between 32-bit and 64-bit memory addressing

**Effect When ENABLED** (`#define MM64 1`):
- Uses 64-bit addresses (`uint64_t`)
- 5-level page table hierarchy:
  - PGD (Page Global Directory)
  - P4D (Page Level 4 Directory)
  - PUD (Page Upper Directory)
  - PMD (Page Middle Directory)
  - PT (Page Table)
- Address space: 128 PiB (petabytes)
- Page size: 4 KB

**Effect When DISABLED** (default):
- Uses 32-bit addresses (`uint32_t`)
- Single-level page table
- Address space: 4 MB
- Page size: 256 bytes

**Memory Structure Changes**:
```c
struct mm_struct {
#ifdef MM64
    uint64_t *pgd;
    uint64_t *p4d;
    uint64_t *pud;
    uint64_t *pmd;
    uint64_t *pt;
#else
    uint32_t *pgd;
#endif
    // ... other fields
};
```

**Address Type Changes**:
```c
#ifdef MM64
#define ADDR_TYPE uint64_t
#define FORMAT_ADDR "%lld"
#else
#define ADDR_TYPE uint32_t
#define FORMAT_ADDR "%d"
#endif
```

**When to use**:
- ✅ Enable for large memory simulations
- ✅ Enable for testing 5-level paging
- ❌ Disable for simple/small simulations (default)

### 5. IODUMP - I/O Operation Dump

```c
#define IODUMP 1
```

**Purpose**: Enable detailed I/O operation logging

**Effect**:
- Prints memory allocation operations
- Shows memory read/write operations
- Displays memory dumps
- Helps with debugging

**When to use**:
- ✅ Enable for debugging memory operations
- ❌ Disable for clean output in production

### 6. PAGETBL_DUMP - Page Table Dump

```c
#define PAGETBL_DUMP 1
```

**Purpose**: Enable page table dumping after operations

**Effect**:
- Prints page table entries after alloc/free/read/write
- Shows PTE flags and mappings
- Useful for understanding page management

**When to use**:
- ✅ Enable for debugging paging operations
- ❌ Disable for clean output

### 7. VMDBG - Virtual Memory Debug

```c
//#define VMDBG 1    // Disabled by default
```

**Purpose**: Enable verbose virtual memory debugging

**Effect**:
- Prints detailed VMA operations
- Shows region allocation/deallocation
- Tracks page fault handling

**When to use**:
- ✅ Enable for debugging virtual memory issues
- ❌ Disable normally (too verbose)

### 8. MMDBG - Memory Management Debug

```c
//#define MMDBG 1    // Disabled by default
```

**Purpose**: Enable memory management debugging

**Effect**:
- Prints internal memory management operations
- Shows frame allocation/deallocation
- Tracks swap operations

**When to use**:
- ✅ Enable for debugging MM internals
- ❌ Disable normally (too verbose)

## Configuration Presets

### Preset 1: Development/Debug Mode

**File**: `include/os-cfg.h`
```c
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
//#define MM_FIXED_MEMSZ     // Use custom memory sizes
#define VMDBG 1              // Enable VM debug
#define MMDBG 1              // Enable MM debug
#define IODUMP 1             // Enable I/O dump
#define PAGETBL_DUMP 1       // Enable page table dump
//#define MM64 1             // 32-bit mode
#undef MM64
```

**Input File**: 3 lines header (custom memory)
```
2 4 8                    # 2 time_slice, 4 CPUs, 8 processes
1048576 16777216 0 0 0   # 1MB RAM, 16MB SWAP0
1 p0s 130
2 s3 39
...
```

### Preset 2: Production/Clean Mode

**File**: `include/os-cfg.h`
```c
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
//#define MM_FIXED_MEMSZ     // Use custom memory sizes
//#define VMDBG 1            // Disable
//#define MMDBG 1            // Disable
//#define IODUMP 1           // Disable
//#define PAGETBL_DUMP 1     // Disable
//#define MM64 1             // 32-bit mode
#undef MM64
```

**Input File**: Same as development

### Preset 3: Backward Compatible Mode

**File**: `include/os-cfg.h`
```c
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
#define MM_FIXED_MEMSZ       // Use fixed memory (IMPORTANT!)
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
//#define MM64 1
#undef MM64
```

**Input File**: 2 lines header (old format)
```
2 4 8                    # 2 time_slice, 4 CPUs, 8 processes
1 p0s 130                # Process list starts immediately
2 s3 39
...
```

### Preset 4: 64-bit Large Memory Mode

**File**: `include/os-cfg.h`
```c
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
//#define MM_FIXED_MEMSZ     // Use custom memory sizes
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
#define MM64 1               // 64-bit mode (IMPORTANT!)
//#undef MM64
```

**Input File**: 3 lines header with larger sizes
```
2 4 8
4194304 67108864 67108864 0 0   # 4MB RAM, 64MB SWAP0, 64MB SWAP1
1 p0s 130
...
```

## Input File Format Details

### Format with MM_FIXED_MEMSZ Disabled (Recommended)

```
Line 1: [time_slice] [num_cpus] [num_processes]
Line 2: [RAM_SIZE] [SWAP0_SIZE] [SWAP1_SIZE] [SWAP2_SIZE] [SWAP3_SIZE]
Line 3+: [time] [process_path] [priority]
```

**Example**: `input/os_1_mlq_paging`
```
2 4 8                      # Time slice=2, CPUs=4, Processes=8
1048576 16777216 0 0 0     # RAM=1MB, SWAP0=16MB, others=0
1 p0s 130                  # Load p0s at time 1, priority 130
2 s3 39                    # Load s3 at time 2, priority 39
4 m1s 15
6 s2 120
7 m0s 120
9 p1s 15
11 s0 38
16 s1 0
```

### Format with MM_FIXED_MEMSZ Enabled (Backward Compatible)

```
Line 1: [time_slice] [num_cpus] [num_processes]
Line 2+: [time] [process_path] [priority]
```

**Example**: `input/sched` (old format)
```
2 1 4                      # Time slice=2, CPUs=1, Processes=4
0 s0 0                     # Load s0 at time 0, priority 0
1 s1 0
2 s2 0
3 s3 0
```

**Note**: Memory sizes are fixed:
- RAM: 1 MB (1048576 bytes)
- SWAP[0]: 16 MB (16777216 bytes)
- SWAP[1-3]: 0 (disabled)

## Memory Size Guidelines

### RAM Size Recommendations

| Configuration | RAM Size | Purpose |
|---------------|----------|---------|
| Small Test | 512 KB | Quick testing |
| Default | 1 MB | Standard simulation |
| Medium | 2 MB | More processes |
| Large | 4 MB | Heavy workload |

### SWAP Size Recommendations

| Configuration | SWAP Size | Ratio | Purpose |
|---------------|-----------|-------|---------|
| Minimal | 4 MB | 4:1 | Light swapping |
| Default | 16 MB | 16:1 | Standard |
| Heavy | 32 MB | 32:1 | Heavy swapping |
| Maximum | 64 MB | 64:1 | Stress test |

### Page Size Configuration

**32-bit mode** (`MM64` disabled):
- Page size: 256 bytes
- Address bus: 22 bits
- Max pages: ~16,000
- Page table size: ~64 KB

**64-bit mode** (`MM64` enabled):
- Page size: 4 KB
- Address bus: 57 bits
- Max pages: millions
- Page table size: varies (multi-level)

## Switching Configurations

### To Switch from 32-bit to 64-bit:

1. Edit `include/os-cfg.h`:
```c
// Change from:
//#define MM64 1
#undef MM64

// To:
#define MM64 1
//#undef MM64
```

2. Rebuild:
```bash
make clean && make
```

3. Update input files if needed (larger memory sizes)

### To Enable Fixed Memory Size:

1. Edit `include/os-cfg.h`:
```c
// Change from:
//#define MM_FIXED_MEMSZ

// To:
#define MM_FIXED_MEMSZ
```

2. Rebuild:
```bash
make clean && make
```

3. Use old format input files (2 lines header)

### To Enable Debug Output:

1. Edit `include/os-cfg.h`:
```c
#define VMDBG 1
#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
```

2. Rebuild:
```bash
make clean && make
```

3. Run with output redirection:
```bash
./os os_1_mlq_paging > debug.log 2>&1
```

## Testing Different Configurations

### Test 1: Default Configuration
```bash
# os-cfg.h: MM_FIXED_MEMSZ disabled, MM64 disabled
make clean && make
./os os_1_mlq_paging
```

### Test 2: Backward Compatible
```bash
# os-cfg.h: MM_FIXED_MEMSZ enabled
make clean && make
./os sched
```

### Test 3: Large Memory
```bash
# os-cfg.h: MM64 enabled
# Create input with large memory sizes
echo "2 4 4" > input/large_test
echo "4194304 67108864 0 0 0" >> input/large_test
echo "1 p0s 130" >> input/large_test
# ... add more processes
make clean && make
./os large_test
```

### Test 4: Debug Mode
```bash
# os-cfg.h: All debug flags enabled
make clean && make
./os os_1_mlq_paging 2>&1 | tee debug_output.txt
```

## Troubleshooting

### Issue: "Cannot find configure file"
**Cause**: Input file path incorrect
**Solution**: Ensure input file is in `input/` directory

### Issue: "Invalid memory configuration"
**Cause**: MM_FIXED_MEMSZ doesn't match input file format
**Solution**: Check header line count (2 vs 3)

### Issue: Compilation errors with MM64
**Cause**: Missing 64-bit definitions
**Solution**: Ensure all `#ifdef MM64` blocks are correct

### Issue: Excessive output
**Cause**: Debug flags enabled
**Solution**: Disable VMDBG, MMDBG, IODUMP, PAGETBL_DUMP

### Issue: Wrong memory size used
**Cause**: MM_FIXED_MEMSZ enabled when it shouldn't be
**Solution**: Comment out `#define MM_FIXED_MEMSZ`

## Best Practices

1. **Always rebuild** after changing os-cfg.h:
```bash
make clean && make
```

2. **Match input format** to MM_FIXED_MEMSZ setting

3. **Use version control** for os-cfg.h changes

4. **Document** which configuration was used for tests

5. **Test incrementally** when changing configurations

6. **Keep backup** of working configurations

## Configuration Checklist

Before running:
- [ ] Check `MM_PAGING` is enabled
- [ ] Verify `MM_FIXED_MEMSZ` matches input file format
- [ ] Confirm `MM64` setting (32-bit or 64-bit)
- [ ] Set debug flags as needed
- [ ] Rebuild with `make clean && make`
- [ ] Verify input file exists and is correct format
- [ ] Check memory sizes are reasonable

## Summary

| Configuration | Purpose | Default | Change |
|---------------|---------|---------|--------|
| `MLQ_SCHED` | Priority scheduling | Enabled | Rarely |
| `MM_PAGING` | Virtual memory | Enabled | Never |
| `MM_FIXED_MEMSZ` | Backward compat | Disabled | For old files |
| `MM64` | 64-bit addressing | Disabled | For large mem |
| `IODUMP` | I/O logging | Enabled | For debug |
| `PAGETBL_DUMP` | Page table log | Enabled | For debug |
| `VMDBG` | VM debug | Disabled | For debug |
| `MMDBG` | MM debug | Disabled | For debug |

The configuration system provides flexibility while maintaining backward compatibility and supporting both educational and advanced use cases.

