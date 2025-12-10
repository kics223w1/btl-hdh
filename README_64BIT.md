# Simple OS - 64-bit Mode Support

This document explains how to compile and run the Simple OS in 64-bit memory addressing mode.

## Overview

The 64-bit mode (`MM64`) enables:
- **64-bit Virtual Addresses**: Using a 5-level page table hierarchy (PGD -> P4D -> PUD -> PMD -> PT).
- **Larger Page Size**: 4KB (4096 bytes) compared to 256 bytes in 32-bit mode.
- **Larger Address Space**: Supports up to 57-bit virtual addresses.
- **Enhanced Memory Structures**: Uses `uint64_t` for page table entries.

## Configuration

To enable 64-bit mode, you must modify the configuration header file before compiling.

1. Open `include/os-cfg.h`.
2. Locate the `MM64` definition section.
3. Uncomment `#define MM64 1` and comment out `#undef MM64`.
4. **Recommended**: Disable `PAGETBL_DUMP` to avoid extremely large output (scanning 128 PiB address space).
   ```c
   //#define PAGETBL_DUMP 1
   ```

**Example `include/os-cfg.h`:**
```c
/* MM64: 64-bit Memory Addressing Mode */
#define MM64 1
//#undef MM64
```

## Compilation

After changing the configuration, you must clean and rebuild the project.

```bash
make clean && make
```

Verify the build is successful. You should see `gcc` commands compiling `mm64.c` and linking the `os` executable.

## Running Tests

You can run the existing tests as usual. The OS simulation will now operate with 64-bit memory management logic.

```bash
./test.sh
```

Or run individual tests:

```bash
./os os_1_mlq_paging
```

## Key Differences

| Feature | 32-bit Mode (Default) | 64-bit Mode (`MM64`) |
|---------|-----------------------|----------------------|
| **Address Width** | 32-bit | 64-bit (57-bit used) |
| **Page Size** | 256 Bytes | 4 KB (4096 Bytes) |
| **Paging Levels** | 1 (Flat Page Table) | 5 (Hierarchical) |
| **PTE Size** | 32-bit | 64-bit |
| **Max RAM** | Limited by 32-bit | Scalable |

## Troubleshooting

### Build Errors
- **"shift count >= width of type"**: Ensure `include/bitops.h` is correctly handling 64-bit types. We have updated it to use `unsigned long long` when `MM64` is enabled.
- **Linker errors**: Ensure `mm64.o` is included in `OS_OBJ` in `Makefile`.

### Runtime Issues
- **Infinite Loops / Hangs**: 64-bit mode requires traversing a deeper page table tree. Ensure your input files provide enough memory steps if simulation seems slow.
- **Memory Errors**: If you see segmentation faults, check if `malloc` is failing for large page tables.

## Implementation Details

The 64-bit implementation is primarily located in:
- `src/mm64.c`: Implements 5-level page table walking and PTE management.
- `include/mm64.h`: Defines 64-bit masks and shifts.
- `include/bitops.h`: Updated to support 64-bit bitwise operations.

