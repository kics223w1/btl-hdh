# Physical Memory System Implementation

## Overview

This document describes the implementation of the physical memory system (section 2.2.2) for the simple OS, including RAM and SWAP device management, frame allocation tracking, and memory statistics.

## Architecture

### System Memory Organization

```
┌─────────────────────────────────────────────────────────────┐
│                    Operating System                         │
│  ┌───────────────────────────────────────────────────────┐  │
│  │              Kernel (krnl_t)                          │  │
│  │  ┌─────────────────────────────────────────────────┐  │  │
│  │  │  mram    → RAM Device (Primary Memory)          │  │  │
│  │  │  mswp[4] → SWAP Devices (Secondary Memory)      │  │  │
│  │  │  active_mswp → Currently active SWAP            │  │  │
│  │  └─────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                          │
                          │ All processes share
                          ↓
┌─────────────────────────────────────────────────────────────┐
│                 Physical Memory Devices                      │
│                                                              │
│  ┌──────────────────┐         ┌──────────────────┐          │
│  │   RAM Device     │         │  SWAP Device 0   │          │
│  │                  │         │                  │          │
│  │ • Random Access  │         │ • Sequential/    │          │
│  │ • Fast           │         │   Random Access  │          │
│  │ • CPU Direct     │         │ • Slower         │          │
│  │ • Limited Size   │         │ • Larger Size    │          │
│  │                  │         │ • No CPU Direct  │          │
│  └──────────────────┘         └──────────────────┘          │
│                                                              │
│  ┌──────────────────┐         ┌──────────────────┐          │
│  │  SWAP Device 1   │   ...   │  SWAP Device 3   │          │
│  └──────────────────┘         └──────────────────┘          │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Physical Memory Device Structure

```c
struct memphy_struct {
   /* Basic fields */
   BYTE *storage;              // Physical storage
   int maxsz;                  // Maximum size in bytes
   
   /* Access mode */
   int rdmflg;                 // 1 = Random, 0 = Sequential
   int cursor;                 // For sequential access
   
   /* Frame management */
   struct framephy_struct *free_fp_list;  // Free frames
   struct framephy_struct *used_fp_list;  // Used frames
};

struct framephy_struct {
   addr_t fpn;                 // Frame Page Number
   struct framephy_struct *fp_next;  // Next frame
   struct mm_struct *owner;    // Owner process
};
```

## Key Features

### 1. RAM Device (Primary Memory)

**Characteristics:**
- Directly accessible from CPU address bus
- Random access memory
- Fast read/write operations
- Limited capacity (typically 1-4 MB in configuration)
- Used for active process pages

**Implementation:**
```c
// Initialized in os.c
struct memphy_struct mram;
init_memphy(&mram, memramsz, 1); // 1 = random access
```

### 2. SWAP Devices (Secondary Memory)

**Characteristics:**
- Up to 4 SWAP devices supported
- Larger capacity (typically 16+ MB each)
- No direct CPU access
- Used for page swapping
- Can be sequential or random access

**Implementation:**
```c
// Initialized in os.c
struct memphy_struct mswp[PAGING_MAX_MMSWP];
for (int i = 0; i < PAGING_MAX_MMSWP; i++)
   init_memphy(&mswp[i], memswpsz[i], 1);
```

## Implemented Functions

### Core Memory Operations

#### 1. MEMPHY_read()
```c
int MEMPHY_read(struct memphy_struct *mp, addr_t addr, BYTE *value);
```
**Purpose**: Read a byte from memory at given address  
**Supports**: Both random and sequential access modes

#### 2. MEMPHY_write()
```c
int MEMPHY_write(struct memphy_struct *mp, addr_t addr, BYTE data);
```
**Purpose**: Write a byte to memory at given address  
**Supports**: Both random and sequential access modes

#### 3. MEMPHY_seq_read() / MEMPHY_seq_write()
```c
int MEMPHY_seq_read(struct memphy_struct *mp, addr_t addr, BYTE *value);
int MEMPHY_seq_write(struct memphy_struct *mp, addr_t addr, BYTE value);
```
**Purpose**: Sequential access with cursor movement  
**Used for**: Serial memory devices

### Frame Allocation Management

#### 4. MEMPHY_get_freefp()
```c
int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn);
```
**Purpose**: Allocate a free frame  
**Action**: Removes frame from free list  
**Returns**: Frame page number in retfpn

#### 5. MEMPHY_put_freefp()
```c
int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn);
```
**Purpose**: Return a frame to free list  
**Action**: Adds frame to beginning of free list

#### 6. MEMPHY_get_usedfp()
```c
int MEMPHY_get_usedfp(struct memphy_struct *mp, addr_t fpn, struct mm_struct *owner);
```
**Purpose**: Allocate frame and track ownership  
**Action**: Moves frame from free list to used list  
**Tracks**: Owner process for debugging/management

#### 7. MEMPHY_put_usedfp()
```c
int MEMPHY_put_usedfp(struct memphy_struct *mp, addr_t fpn, struct mm_struct *owner);
```
**Purpose**: Add frame to used list  
**Action**: Adds frame with owner tracking

#### 8. MEMPHY_remove_usedfp()
```c
int MEMPHY_remove_usedfp(struct memphy_struct *mp, addr_t fpn);
```
**Purpose**: Remove frame from used list  
**Returns**: 0 if found and removed, -1 otherwise

#### 9. MEMPHY_free_usedfp()
```c
int MEMPHY_free_usedfp(struct memphy_struct *mp, addr_t fpn);
```
**Purpose**: Free a used frame  
**Action**: Moves frame from used list back to free list

### Statistics and Monitoring

#### 10. MEMPHY_dump()
```c
int MEMPHY_dump(struct memphy_struct *mp);
```
**Purpose**: Comprehensive memory dump for debugging  
**Displays**:
- Device configuration (size, access mode)
- Storage content (hexadecimal dump)
- Free frame list
- Used frame list with ownership
- Cursor position (for sequential devices)

**Example Output:**
```
=== MEMPHY DUMP ===
Max Size: 1048576 bytes
Access Mode: Random
Storage Content (first 256 bytes):
0000: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
...
Free Frame List:
  FPN: 0, FPN: 1, FPN: 2, FPN: 3, FPN: 4,
  ...
Total Free Frames Shown: 4095

Used Frame List:
  FPN: 100 (owner: 0x7ffee123), FPN: 101 (owner: 0x7ffee123),
  ...
Total Used Frames Shown: 5
===================
```

#### 11. MEMPHY_get_frame_count()
```c
int MEMPHY_get_frame_count(struct framephy_struct *fp_list);
```
**Purpose**: Count frames in a list  
**Returns**: Number of frames

#### 12. MEMPHY_get_stats()
```c
int MEMPHY_get_stats(struct memphy_struct *mp, 
                     int *free_frames, 
                     int *used_frames, 
                     int *total_frames);
```
**Purpose**: Get memory usage statistics  
**Returns**: Frame counts via output parameters

#### 13. MEMPHY_print_stats()
```c
int MEMPHY_print_stats(struct memphy_struct *mp, const char *name);
```
**Purpose**: Print formatted statistics  
**Example Output:**
```
=== RAM Statistics ===
Total Size: 1048576 bytes (1024 KB)
Total Frames: 4096
Free Frames: 4091 (99.9%)
Used Frames: 5 (0.1%)
Access Mode: Random
======================
```

### Search and Validation

#### 14. MEMPHY_find_frame()
```c
int MEMPHY_find_frame(struct memphy_struct *mp, addr_t fpn, struct mm_struct **owner);
```
**Purpose**: Find a frame in used list  
**Returns**: 0 if found with owner, -1 if not found

#### 15. MEMPHY_is_frame_free()
```c
int MEMPHY_is_frame_free(struct memphy_struct *mp, addr_t fpn);
```
**Purpose**: Check if frame is in free list  
**Returns**: 1 if free, 0 if not

#### 16. MEMPHY_validate()
```c
int MEMPHY_validate(struct memphy_struct *mp);
```
**Purpose**: Validate memory device integrity  
**Checks**:
- NULL pointer validation
- Storage allocation
- Size validity
- Frame count consistency
- No duplicate frames

### Cleanup

#### 17. MEMPHY_cleanup()
```c
int MEMPHY_cleanup(struct memphy_struct *mp);
```
**Purpose**: Free all allocated memory  
**Cleans**:
- Storage array
- Free frame list
- Used frame list

## Device Configuration

### Supported Settings

| Setting | RAM | SWAP |
|---------|-----|------|
| **Size** | Configurable (typically 1-4 MB) | Configurable (typically 16+ MB) |
| **Access Mode** | Random (rdmflg = 1) | Random or Sequential |
| **CPU Direct Access** | Yes | No |
| **Usage** | Active pages | Swapped pages |
| **Speed** | Fast | Slower |
| **Count** | 1 device | Up to 4 devices |

### Configuration Example

From `input/os_1_mlq_paging`:
```
2 4 8                    # Number of CPUs and other settings
1048576 16777216 0 0 0   # RAM size: 1MB, SWAP0: 16MB
```

## Frame Management Flow

### Allocation Flow

```
1. Process requests memory
        ↓
2. MEMPHY_get_freefp(RAM)
        ↓
3. Check if frame available
        ↓
   ┌────┴────┐
   │ Yes     │ No → Need page swapping
   ↓         ↓
4. Remove from free_fp_list
   Add to used_fp_list
        ↓
5. Track owner (process mm_struct)
        ↓
6. Return frame number
```

### Deallocation Flow

```
1. Process frees memory
        ↓
2. MEMPHY_free_usedfp(RAM, fpn)
        ↓
3. Find in used_fp_list
        ↓
4. Remove from used list
        ↓
5. Clear frame content (optional)
        ↓
6. Add to free_fp_list
```

### Page Swapping Flow

```
1. RAM full, need new frame
        ↓
2. Select victim page (FIFO/LRU)
        ↓
3. Get free frame from SWAP
   MEMPHY_get_freefp(SWAP)
        ↓
4. Copy page: RAM → SWAP
   __swap_cp_page(RAM, vicfpn, SWAP, swpfpn)
        ↓
5. Update page table entry
   pte_set_swap(pgn, SWAP_TYPE, swpfpn)
        ↓
6. Free RAM frame
   MEMPHY_free_usedfp(RAM, vicfpn)
        ↓
7. Use freed RAM frame
```

## Advantages of the Implementation

### 1. **Separation of Primary and Secondary Memory**
- RAM for fast, direct CPU access
- SWAP for extended virtual memory
- Clear distinction in usage patterns

### 2. **Frame Ownership Tracking**
- Each frame knows its owner process
- Facilitates debugging and resource management
- Enables process-specific memory statistics

### 3. **Flexible Device Configuration**
- Support for multiple SWAP devices
- Configurable sizes
- Random or sequential access modes

### 4. **Comprehensive Monitoring**
- Real-time statistics
- Memory dumps for debugging
- Frame tracking and validation

### 5. **Efficient Frame Management**
- Separate free and used lists
- Fast allocation/deallocation
- No memory fragmentation in frame lists

### 6. **Error Detection**
- Validation functions
- Consistency checks
- Duplicate detection

### 7. **Resource Cleanup**
- Proper memory deallocation
- No memory leaks
- Clean shutdown

## Usage Examples

### Example 1: Initialize RAM and SWAP

```c
// Initialize RAM device (1 MB, random access)
struct memphy_struct mram;
init_memphy(&mram, 1048576, 1);

// Initialize SWAP devices (16 MB each, random access)
struct memphy_struct mswp[4];
for (int i = 0; i < 4; i++)
   init_memphy(&mswp[i], 16777216, 1);
```

### Example 2: Allocate and Use a Frame

```c
// Allocate a frame from RAM
addr_t fpn;
if (MEMPHY_get_freefp(&mram, &fpn) == 0)
{
   // Write data to the frame
   addr_t addr = fpn * PAGING_PAGESZ;
   for (int i = 0; i < PAGING_PAGESZ; i++)
   {
      MEMPHY_write(&mram, addr + i, (BYTE)i);
   }
   
   // Track usage
   MEMPHY_put_usedfp(&mram, fpn, process->krnl->mm);
}
```

### Example 3: Print Memory Statistics

```c
// Print RAM statistics
MEMPHY_print_stats(&mram, "RAM");

// Print SWAP statistics
for (int i = 0; i < 4; i++)
{
   char name[16];
   sprintf(name, "SWAP%d", i);
   MEMPHY_print_stats(&mswp[i], name);
}
```

### Example 4: Swap a Page

```c
// Get victim page from RAM
addr_t vicfpn = ...; // From page replacement algorithm

// Get free frame from SWAP
addr_t swpfpn;
if (MEMPHY_get_freefp(active_swap, &swpfpn) == 0)
{
   // Copy page content
   __swap_cp_page(&mram, vicfpn, active_swap, swpfpn);
   
   // Free RAM frame
   MEMPHY_free_usedfp(&mram, vicfpn);
   
   // Track SWAP usage
   MEMPHY_put_usedfp(active_swap, swpfpn, process->krnl->mm);
}
```

## Testing and Validation

### Validation Functions

```c
// Validate memory device
if (MEMPHY_validate(&mram) < 0)
{
   printf("RAM device validation failed!\n");
}

// Check frame availability
if (MEMPHY_is_frame_free(&mram, fpn))
{
   printf("Frame %d is available\n", fpn);
}

// Find frame owner
struct mm_struct *owner;
if (MEMPHY_find_frame(&mram, fpn, &owner) == 0)
{
   printf("Frame %d owned by process %p\n", fpn, owner);
}
```

### Debug Output

```c
// Dump complete memory state
MEMPHY_dump(&mram);

// Print statistics
int free, used, total;
MEMPHY_get_stats(&mram, &free, &used, &total);
printf("Memory utilization: %.1f%%\n", 
       (float)used * 100.0 / total);
```

## Build and Test

### Compilation

```bash
$ cd /Applications/dev/btl-hdh
$ make clean && make
```

**Result**: ✅ Compiles successfully with no errors

### Running Tests

```bash
$ ./os os_1_mlq_paging
```

The system will initialize RAM and SWAP devices and manage physical memory throughout process execution.

## Performance Considerations

### RAM Access
- **Random access**: O(1) for read/write
- **Frame allocation**: O(1) from free list
- **Frame search**: O(n) in used list

### SWAP Access
- **Random access**: O(1) for read/write
- **Sequential access**: O(n) for seek operations
- **Page swap**: O(page_size) for copying

### Memory Overhead
- **Per frame**: ~16-24 bytes for framephy_struct
- **Per device**: ~24-32 bytes for memphy_struct
- **Total overhead**: < 1% of physical memory

## Future Enhancements

1. **Wear Leveling**: For SSD-backed SWAP devices
2. **Compression**: Compress pages in SWAP
3. **Prefetching**: Anticipatory page loading
4. **NUMA Support**: Non-uniform memory access
5. **Memory Pools**: Specialized allocators for different page types
6. **Hot/Cold Pages**: Separate management for frequently/rarely used pages
7. **DMA Support**: Direct memory access for I/O devices

## Conclusion

The physical memory implementation provides a robust foundation for memory management in the simple OS, with clear separation between primary (RAM) and secondary (SWAP) storage, comprehensive tracking and monitoring, and efficient frame management algorithms that mirror real-world operating systems.

