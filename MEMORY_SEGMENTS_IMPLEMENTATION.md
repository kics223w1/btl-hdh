# Multiple Memory Segments Implementation

## Overview

This document describes the implementation of the multiple memory segments design in the simple OS, as specified in section 2.2 of the project requirements.

## Architecture

### Key Structures

#### 1. Memory Region (`vm_rg_struct`)
Represents actual allocated portions within a memory area, corresponding to variables/data structures in a program.

```c
struct vm_rg_struct {
   addr_t rg_start;      // Start address of the region
   addr_t rg_end;        // End address of the region
   struct vm_rg_struct *rg_next;  // Linked list pointer
};
```

#### 2. Memory Area (`vm_area_struct`)
Represents a contiguous virtual memory area (e.g., code, stack, heap segments).

```c
struct vm_area_struct {
   unsigned long vm_id;           // Unique identifier
   addr_t vm_start;               // Start of virtual address range
   addr_t vm_end;                 // End of virtual address range
   addr_t sbrk;                   // Current break pointer (usable limit)
   struct mm_struct *vm_mm;       // Back pointer to memory manager
   struct vm_rg_struct *vm_freerg_list;  // Free regions list
   struct vm_area_struct *vm_next;       // Next VMA in list
};
```

#### 3. Memory Management (`mm_struct`)
Top-level structure managing all memory areas and page tables for a process.

```c
struct mm_struct {
   uint64_t *pgd;  // Page Global Directory (5-level paging)
   uint64_t *p4d;  // Page Level 4 Directory
   uint64_t *pud;  // Page Upper Directory
   uint64_t *pmd;  // Page Middle Directory
   uint64_t *pt;   // Page Table
   
   struct vm_area_struct *mmap;  // List of memory areas
   struct vm_rg_struct symrgtbl[PAGING_MAX_SYMTBL_SZ];  // Symbol table
   struct pgn_t *fifo_pgn;  // FIFO page list for replacement
};
```

## Implemented Features

### 1. Memory Range Checking (include/mm.h)

Two essential macros for managing multiple memory segments:

- **INCLUDE**: Checks if range [y1, y2] is completely within [x1, x2]
  ```c
  #define INCLUDE(x1,x2,y1,y2) ((x1) <= (y1) && (y2) <= (x2))
  ```

- **OVERLAP**: Checks if two ranges overlap (crucial for preventing conflicts)
  ```c
  #define OVERLAP(x1,x2,y1,y2) (((x1) < (y2)) && ((y1) < (x2)))
  ```

### 2. VMA Expansion (src/mm-vm.c)

**inc_vma_limit()**: Increases the limit of a virtual memory area to accommodate growth.

Features:
- Automatic page alignment
- Overlap validation with other VMAs
- Rollback on failure
- Physical memory mapping

```c
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
```

### 3. Multiple VMA Management Functions (src/mm-vm.c)

#### create_vm_area()
Creates a new virtual memory area with specified boundaries.

```c
struct vm_area_struct *create_vm_area(int vmaid, addr_t vm_start, addr_t vm_end)
```

Features:
- Initializes all VMA fields
- Creates initial free region list
- Ready for insertion into mm_struct

#### add_vm_area()
Adds a VMA to the memory management structure with overlap checking.

```c
int add_vm_area(struct mm_struct *mm, struct vm_area_struct *new_vma)
```

Features:
- Validates no overlap with existing VMAs
- Maintains sorted order by vm_id
- Links VMA back to mm_struct

#### remove_vm_area()
Safely removes a VMA and frees associated resources.

```c
int remove_vm_area(struct mm_struct *mm, int vmaid)
```

Features:
- Finds and unlinks VMA from list
- Frees all associated regions
- Maintains list integrity

#### merge_vm_areas()
Merges two adjacent VMAs into one.

```c
int merge_vm_areas(struct vm_area_struct *vma1, struct vm_area_struct *vma2)
```

Features:
- Validates adjacency
- Combines free region lists
- Updates boundaries and sbrk

#### split_vm_area()
Splits a VMA at a specified address.

```c
int split_vm_area(struct vm_area_struct *vma, addr_t split_addr, 
                  struct vm_area_struct **new_vma)
```

Features:
- Creates new VMA for upper portion
- Splits free region lists appropriately
- Maintains list linkage

### 4. Enhanced Memory Initialization (src/mm64.c)

**init_mm()**: Initializes memory management with support for multiple VMAs.

Features:
- Allocates 5-level page table structure (64-bit)
- Creates initial heap VMA (vm_id = 0)
- Initializes symbol region table
- Sets up FIFO page replacement tracking

### 5. Multi-Level Page Table Operations (src/mm64.c)

#### pte_set_fpn() & pte_set_swap()
Navigate 5-level page table hierarchy and set page table entries.

Features:
- On-demand allocation of page table levels
- Proper initialization of new table levels
- Support for both present and swapped pages

#### vmap_page_range()
Maps a range of physical frames to virtual pages.

Features:
- Iterates through frame list
- Sets PTEs for each page
- Tracks pages for replacement policy

#### alloc_pages_range()
Allocates physical frames from RAM.

Features:
- Requests frames from memory physical layer
- Builds frame list
- Returns appropriate error codes

#### print_pgtbl()
Dumps page table contents for debugging.

Features:
- Traverses multi-level page table
- Shows page directory indices
- Displays PTE flags and mappings

## Advantages of Multiple Memory Segments Design

### 1. **Logical Separation and Organization**
Different types of memory (code, stack, heap) are separated into distinct areas, mirroring real-world process organization.

### 2. **Independent Growth**
Each VMA can grow independently without affecting others:
- Heap expands upward via sbrk
- Stack can grow downward
- Each tracks its own free regions

### 3. **Efficient Memory Utilization**
- Physical memory allocated only for usable area (up to sbrk)
- Free region tracking within each VMA
- No waste on unreachable virtual space

### 4. **Memory Protection**
- Different segments can have different permissions
- Easier to implement execute-only code segments
- Write-protected data segments

### 5. **Flexibility**
- Non-contiguous virtual areas
- Support for memory-mapped files
- Shared memory regions
- Address Space Layout Randomization (ASLR)

### 6. **Scalability**
Linked list structure allows arbitrary number of memory areas without pre-allocating large structures.

### 7. **Reduced Fragmentation**
Only allocating up to sbrk reduces internal fragmentation.

### 8. **Real-World Compatibility**
Design aligns with production OS (Linux VMA, Windows VAD), making it educational and practical.

## Usage Example

```c
// Create a new memory area for stack
struct vm_area_struct *stack_vma = create_vm_area(1, 0x10000, 0x20000);

// Add it to the process memory management
add_vm_area(process->krnl->mm, stack_vma);

// Expand the heap area
inc_vma_limit(process, 0, 4096);  // Grow heap by 4KB

// Remove a VMA when done
remove_vm_area(process->krnl->mm, 1);
```

## Architecture Diagram

```
Process Memory Layout:
┌─────────────────────────────────────┐
│       mm_struct (Memory Manager)    │
│  ┌────────────────────────────────┐ │
│  │ pgd → 5-Level Page Tables      │ │
│  │ mmap → VMA List                │ │
│  │ symrgtbl[30] → Symbol Regions  │ │
│  │ fifo_pgn → Page Replacement    │ │
│  └────────────────────────────────┘ │
└──────────────┬──────────────────────┘
               │
               ↓
        ┌─────────────┐
        │   VMA #0    │ (Heap)
        │ [0 - 0x1000]│
        │ sbrk: 0x500 │
        └──────┬──────┘
               ↓
        ┌─────────────┐
        │   VMA #1    │ (Stack)
        │[0x10000-... ]│
        │ sbrk: 0x10000│
        └──────┬──────┘
               ↓
             (null)

Each VMA contains:
- Free region list
- Allocated regions
- Current usage (sbrk)
```

## Testing

The implementation can be tested with:

```bash
# Compile the OS
make clean && make

# Run test scenarios
./os < input/os_1_mlq_paging
```

## Future Enhancements

1. **Dynamic VMA Creation**: Automatic VMA creation for mmap operations
2. **Copy-on-Write**: Efficient process forking with shared VMAs
3. **Demand Paging**: Lazy allocation of physical frames
4. **Huge Pages**: Support for large page sizes
5. **NUMA Support**: Non-uniform memory access awareness

## Conclusion

This implementation provides a robust foundation for memory management in a simple operating system, demonstrating key concepts of virtual memory, paging, and memory segmentation used in modern operating systems.

