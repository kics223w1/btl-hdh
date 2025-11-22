# Simple Operating System

Multi-Level Queue Scheduler + Memory Management System

## Build

```bash
make clean && make
```

## Run

```bash
./os os_1_mlq_paging
./os os_1_singleCPU_mlq_paging  
./os os_0_mlq_paging
```

## Test

```bash
# Compare with expected output
./os os_1_mlq_paging > my_output.txt
diff my_output.txt output/os_1_mlq_paging.output
```

**Note**: Output may differ due to concurrent execution - this is normal.

## Configuration

Edit `include/os-cfg.h` then rebuild with `make clean && make`

### Key Options

```c
#define MLQ_SCHED 1          // ✅ Multi-level queue scheduler
#define MM_PAGING            // ✅ Paging memory management
#define MM64 1               // ✅ 64-bit addressing (default)
//#define MM_FIXED_MEMSZ     // ❌ Custom memory sizes (recommended)
#define IODUMP 1             // ✅ I/O logging
#define PAGETBL_DUMP 1       // ✅ Page table dumps
```

## Input File Format

```
[time_slice] [num_cpus] [num_processes]
[RAM_SIZE] [SWAP0] [SWAP1] [SWAP2] [SWAP3]
[time] [path] [priority]
...
```

Example (`input/os_1_mlq_paging`):
```
2 4 8
1048576 16777216 0 0 0
1 p0s 130
2 s3 39
...
```

## Implementation

**Scheduler** (Section 2.1):
- `enqueue()`, `dequeue()` - Queue operations
- `get_mlq_proc()` - MLQ policy with 140 priorities

**Memory Management** (Section 2.2):
- Virtual memory with multiple segments
- Physical memory (RAM + SWAP)
- Paging and address translation
- Configuration system

**Synchronization** (Section 2.3):
- `queue_lock` - Scheduler protection
- `mmvm_lock` - Memory protection

## Documentation

See additional `.md` files for detailed documentation (290+ pages total).

---

**Course**: CO2018 - Operating Systems  
**Institution**: HCMC University of Technology VNU-HCM
