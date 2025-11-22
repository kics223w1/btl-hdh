# Paging-Based Address Translation Implementation

## Overview

This document describes the complete implementation of section 2.2.3: Paging-based Address Translation Scheme, including virtual-to-physical address translation, page swapping mechanisms, and memory operations (ALLOC, FREE, READ, WRITE).

## Architecture

### System Components

```
┌──────────────────────────────────────────────────────────────┐
│                     User Space (libmem)                       │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  Application Memory Operations                         │  │
│  │  • liballoc() - Allocate memory                        │  │
│  │  • libfree() - Free memory                             │  │
│  │  • libread() - Read from memory                        │  │
│  │  • libwrite() - Write to memory                        │  │
│  └────────────────────┬───────────────────────────────────┘  │
└───────────────────────┼──────────────────────────────────────┘
                        │ Library Calls
                        ↓
┌──────────────────────────────────────────────────────────────┐
│              Memory Management Layer (mm-vm)                  │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  Virtual Memory Operations                             │  │
│  │  • __alloc() - Region allocation                       │  │
│  │  • __free() - Region deallocation                      │  │
│  │  • __read() - Virtual address read                     │  │
│  │  • __write() - Virtual address write                   │  │
│  │  • pg_getpage() - Ensure page in RAM                   │  │
│  │  • pg_getval() - Read virtual address                  │  │
│  │  • pg_setval() - Write virtual address                 │  │
│  └────────────────────┬───────────────────────────────────┘  │
└───────────────────────┼──────────────────────────────────────┘
                        │ Syscalls
                        ↓
┌──────────────────────────────────────────────────────────────┐
│           Kernel (Memory Management Unit)                     │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  Page Table Management (mm)                            │  │
│  │  • pte_get_entry() - Get page table entry              │  │
│  │  • pte_set_entry() - Set page table entry              │  │
│  │  • pte_set_fpn() - Map page to frame                   │  │
│  │  • pte_set_swap() - Mark page as swapped               │  │
│  │  • vmap_page_range() - Map virtual pages               │  │
│  └────────────────────┬───────────────────────────────────┘  │
└───────────────────────┼──────────────────────────────────────┘
                        │
                        ↓
┌──────────────────────────────────────────────────────────────┐
│           Physical Memory Layer (memphy)                      │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  RAM Device              SWAP Devices                  │  │
│  │  • MEMPHY_read()         • MEMPHY_read()               │  │
│  │  • MEMPHY_write()        • MEMPHY_write()              │  │
│  │  • MEMPHY_get_freefp()   • MEMPHY_get_freefp()         │  │
│  │  • MEMPHY_put_freefp()   • MEMPHY_put_freefp()         │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

## Page Table Entry (PTE) Format

### 32-bit PTE Structure

```
31 30 29 28    27-15      14-13    12-0
┌──┬──┬──┬──┬──────────┬──────┬──────────┐
│P │S │R │D │  USRNUM  │ 00   │   FPN    │  If PRESENT
└──┴──┴──┴──┴──────────┴──────┴──────────┘
31 30 29 28    27-15      14-13   12-5    4-0
┌──┬──┬──┬──┬──────────┬──────┬────────┬─────┐
│P │S │R │D │  USRNUM  │ 00   │ SWPOFF │ TYP │  If SWAPPED
└──┴──┴──┴──┴──────────┴──────┴────────┴─────┘
```

### Bit Fields

| Bits | Name | Description |
|------|------|-------------|
| 31 | P (Present) | 1 = Page in RAM, 0 = Not present |
| 30 | S (Swapped) | 1 = Page in SWAP, 0 = Not swapped |
| 29 | R (Reserved) | Reserved for future use |
| 28 | D (Dirty) | 1 = Page modified, 0 = Clean |
| 27-15 | USRNUM | User-defined numbering |
| 14-13 | Zero | Always zero if present |
| 12-0 | FPN | Frame Page Number (if present) |
| 25-5 | SWPOFF | Swap offset (if swapped) |
| 4-0 | SWPTYP | Swap type (if swapped) |

## Address Translation

### Virtual to Physical Address Translation

```
Virtual Address (22-bit example):
┌────────────┬──────────┐
│ Page Num   │  Offset  │
│  (14 bits) │ (8 bits) │
└────────────┴──────────┘

Translation Process:
1. Extract page number (PGN) from virtual address
2. Use PGN as index into page table
3. Check PTE:
   - If PRESENT bit = 1: Get FPN from PTE
   - If PRESENT bit = 0: PAGE FAULT → pg_getpage()
4. Calculate physical address: (FPN << FPN_SHIFT) + OFFSET
5. Access physical memory at calculated address
```

### Example Translation

```c
// Virtual address: 0x1234 (22-bit mode, 256B pages)
// Binary: 00 0100 1000 1101 00

// Extract components:
PGN = (0x1234 >> 8) = 0x12 = 18
OFFSET = (0x1234 & 0xFF) = 0x34 = 52

// Lookup page table:
PTE = page_table[18]

// If PTE = 0x80000005:
// Binary: 1000 0000 0000 0000 0000 0000 0000 0101
// Present = 1, Swapped = 0
// FPN = 0x005 = 5

// Calculate physical address:
Physical Address = (5 << 8) + 52
                 = 0x500 + 0x34
                 = 0x534
```

## Implemented Functions

### 1. pg_getpage() - Ensure Page in RAM

**Purpose**: Ensure a virtual page is loaded in RAM, handling page faults and swapping

**Algorithm**:
```
1. Get PTE for page
2. If page is PRESENT:
   - Return FPN from PTE
3. If page is NOT PRESENT:
   a. Check if page was swapped:
      - If SWAPPED bit set: get swap offset
      - If not swapped: treat as new page (zero-fill)
   
   b. Try to get free frame in RAM:
      - If available: use it
      - If RAM full:
        i. Find victim page (FIFO algorithm)
        ii. Get victim's FPN
        iii. Get free frame in SWAP
        iv. Copy victim page: RAM → SWAP
        v. Update victim PTE (mark as swapped)
        vi. Use freed frame
   
   c. If page was swapped:
      - Copy page: SWAP → RAM
      - Free SWAP frame
   
   d. If page was new:
      - Zero-fill the frame
   
   e. Update PTE (mark as present, set FPN)
   f. Add page to FIFO list
4. Return FPN
```

**Implementation**:
```c
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = pte_get_entry(caller, pgn);

  if (!PAGING_PAGE_PRESENT(pte))
  {
    // Handle page fault
    addr_t tgtfpn; // Target frame
    
    // Try to get free frame
    if (MEMPHY_get_freefp(caller->krnl->mram, &tgtfpn) == -1)
    {
      // RAM full - swap out victim
      addr_t vicpgn, vicfpn, victim_swpfpn;
      find_victim_page(caller->krnl->mm, &vicpgn);
      vicfpn = PAGING_FPN(pte_get_entry(caller, vicpgn));
      MEMPHY_get_freefp(caller->krnl->active_mswp, &victim_swpfpn);
      
      // Swap victim to SWAP
      __mm_swap_page(caller, vicfpn, victim_swpfpn);
      pte_set_swap(caller, vicpgn, swap_type, victim_swpfpn);
      tgtfpn = vicfpn;
    }
    
    // If page was in SWAP, load it
    if (pte & PAGING_PTE_SWAPPED_MASK)
    {
      addr_t swpfpn = PAGING_SWP(pte);
      __mm_swap_page(caller, tgtfpn, swpfpn);
      MEMPHY_put_freefp(caller->krnl->active_mswp, swpfpn);
    }
    else
    {
      // New page - zero fill
      // ... zero fill code ...
    }
    
    // Mark page as present
    pte_set_fpn(caller, pgn, tgtfpn);
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
  }
  
  *fpn = PAGING_FPN(pte_get_entry(caller, pgn));
  return 0;
}
```

### 2. pg_getval() - Read from Virtual Address

**Purpose**: Read a byte from virtual memory

**Algorithm**:
```
1. Extract page number and offset from virtual address
2. Call pg_getpage() to ensure page is in RAM
3. Calculate physical address = (FPN << SHIFT) + OFFSET
4. Read from physical memory via syscall
5. Return value
```

**Implementation**:
```c
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Ensure page is in RAM
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  // Calculate physical address
  addr_t phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  // Read via syscall
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  syscall(caller->krnl, caller->pid, 17, &regs);
  
  *data = (BYTE)regs.a3;
  return 0;
}
```

### 3. pg_setval() - Write to Virtual Address

**Purpose**: Write a byte to virtual memory

**Algorithm**:
```
1. Extract page number and offset from virtual address
2. Call pg_getpage() to ensure page is in RAM
3. Calculate physical address = (FPN << SHIFT) + OFFSET
4. Write to physical memory via syscall
5. Mark page as dirty in PTE
```

**Implementation**:
```c
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  // Ensure page is in RAM
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  // Calculate physical address
  addr_t phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  // Write via syscall
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;
  syscall(caller->krnl, caller->pid, 17, &regs);

  // Mark page as dirty
  uint32_t pte = pte_get_entry(caller, pgn);
  pte |= PAGING_PTE_DIRTY_MASK;
  pte_set_entry(caller, pgn, pte);
  
  return 0;
}
```

## Memory Operations

### 1. ALLOC - Memory Allocation

**Purpose**: Allocate a memory region for a variable

**Flow Diagram**:
```
liballoc(proc, size, reg_index)
         ↓
__alloc(caller, vmaid=0, rgid, size, &addr)
         ↓
   ┌─────┴─────┐
   │           │
   ↓           ↓
Find free    No free
region       region
   │           │
   ↓           ↓
Use it     Expand VMA
   │       (inc_vma_limit)
   │           ↓
   │      Allocate frames
   │           ↓
   │      Map pages
   │           │
   └───────┬───┘
           ↓
  Update symbol table
           ↓
  Return address
```

**Key Steps**:
1. Try to find free region in `vm_freerg_list`
2. If found: reuse existing space
3. If not found:
   - Calculate required pages
   - Call syscall to increase VMA limit
   - Allocate physical frames
   - Map virtual pages to frames
4. Update symbol region table
5. Return allocated virtual address

### 2. FREE - Memory Deallocation

**Purpose**: Free a previously allocated memory region

**Flow Diagram**:
```
libfree(proc, reg_index)
         ↓
__free(caller, vmaid=0, rgid)
         ↓
Get region from symbol table
         ↓
Validate region
         ↓
Create free region node
         ↓
Add to vm_freerg_list
         ↓
Clear symbol table entry
```

**Key Steps**:
1. Get region info from symbol table using rgid
2. Validate region exists
3. Create a free region node
4. Add to process's free region list
5. Clear symbol table entry
6. Physical frames NOT returned (avoid fragmentation)

### 3. READ - Memory Read

**Purpose**: Read data from allocated memory

**Flow Diagram**:
```
libread(proc, source, offset, &dest)
         ↓
__read(caller, vmaid=0, rgid, offset, &data)
         ↓
Get region from symbol table
         ↓
Calculate virtual address
 = region.start + offset
         ↓
pg_getval(mm, vaddr, &data, caller)
         ↓
   pg_getpage() - ensure in RAM
         ↓
   Calculate physical address
         ↓
   MEMPHY_read() via syscall
         ↓
Return data
```

**Key Steps**:
1. Get region from symbol table
2. Calculate virtual address = region_start + offset
3. Ensure page is in RAM (pg_getpage)
4. Translate to physical address
5. Read from physical memory
6. Return value

### 4. WRITE - Memory Write

**Purpose**: Write data to allocated memory

**Flow Diagram**:
```
libwrite(proc, data, dest, offset)
         ↓
__write(caller, vmaid=0, rgid, offset, value)
         ↓
Get region from symbol table
         ↓
Calculate virtual address
 = region.start + offset
         ↓
pg_setval(mm, vaddr, value, caller)
         ↓
   pg_getpage() - ensure in RAM
         ↓
   Calculate physical address
         ↓
   MEMPHY_write() via syscall
         ↓
   Mark page as dirty
```

**Key Steps**:
1. Get region from symbol table
2. Calculate virtual address = region_start + offset
3. Ensure page is in RAM (pg_getpage)
4. Translate to physical address
5. Write to physical memory
6. Mark page as dirty in PTE

## Page Swapping Mechanism

### Swapping Out (RAM → SWAP)

```
Victim page in RAM needs to be moved to SWAP

1. Find victim page using FIFO
   • Get oldest page from fifo_pgn list
   
2. Get victim page's FPN from PTE
   
3. Allocate frame in SWAP
   • MEMPHY_get_freefp(SWAP, &swpfpn)
   
4. Copy page content
   • __swap_cp_page(RAM, vicfpn, SWAP, swpfpn)
   • Copy PAGING_PAGESZ bytes
   
5. Update victim PTE
   • Clear PRESENT bit
   • Set SWAPPED bit
   • Store swap offset
   • pte_set_swap(vicpgn, swptyp, swpoff)
   
6. Free RAM frame
   • MEMPHY_put_freefp(RAM, vicfpn)
```

### Swapping In (SWAP → RAM)

```
Page in SWAP needs to be loaded to RAM

1. Get page's swap offset from PTE
   • swpfpn = PAGING_SWP(pte)
   
2. Allocate frame in RAM
   • MEMPHY_get_freefp(RAM, &fpn)
   • May require swapping out another page
   
3. Copy page content
   • __swap_cp_page(SWAP, swpfpn, RAM, fpn)
   
4. Update PTE
   • Set PRESENT bit
   • Clear SWAPPED bit
   • Store FPN
   • pte_set_fpn(pgn, fpn)
   
5. Free SWAP frame
   • MEMPHY_put_freefp(SWAP, swpfpn)
   
6. Add to FIFO list
   • enlist_pgn_node(&mm->fifo_pgn, pgn)
```

### FIFO Page Replacement

```
FIFO List: [Most Recent] → ... → [Oldest]

When page loaded:
  • Add to head of list
  • enlist_pgn_node(&mm->fifo_pgn, pgn)

When victim needed:
  • Remove from tail of list
  • find_victim_page(mm, &vicpgn)
  • Returns oldest page

List Structure:
┌────┐    ┌────┐    ┌────┐    ┌────┐
│PGN │ →  │PGN │ →  │PGN │ →  │PGN │ → NULL
│ 5  │    │ 3  │    │ 7  │    │ 2  │
└────┘    └────┘    └────┘    └────┘
 newest                        oldest
                              (victim)
```

## Module Integration

### Call Flow for Memory Allocation

```
Application Code:
  int *ptr = (int*)malloc(sizeof(int));

↓ Library Level (libmem.c)
  liballoc(proc, sizeof(int), reg_index)
    ↓
  __alloc(proc, 0, reg_index, size, &addr)
    ↓
  get_free_vmrg_area() or inc_vma_limit()

↓ Syscall Layer
  syscall(krnl, pid, 17, &regs)  // SYSMEM_INC_OP

↓ Kernel Level (mm-vm.c, mm64.c)
  inc_vma_limit(caller, vmaid, inc_sz)
    ↓
  vm_map_ram(caller, start, end, mapstart, pgnum, rg)
    ↓
  alloc_pages_range(caller, pgnum, &frm_lst)
    ↓
  vmap_page_range(caller, addr, pgnum, frames, ret_rg)
    ↓
  pte_set_fpn(caller, pgn, fpn)

↓ Physical Layer (mm-memphy.c)
  MEMPHY_get_freefp(mram, &fpn)
```

### Call Flow for Memory Read

```
Application Code:
  int value = *ptr;

↓ Library Level (libmem.c)
  libread(proc, source, offset, &dest)
    ↓
  __read(proc, 0, source, offset, &data)
    ↓
  pg_getval(mm, vaddr, &data, caller)
    ↓
  pg_getpage(mm, pgn, &fpn, caller)
    │
    ├─ Page Present? → Get FPN
    │
    └─ Page Fault?
         ↓
       Find victim, swap out if needed
         ↓
       Load page from SWAP (or zero-fill)
         ↓
       Update PTE

↓ Syscall Layer
  syscall(krnl, pid, 17, &regs)  // SYSMEM_IO_READ

↓ Physical Layer (mm-memphy.c)
  MEMPHY_read(mram, phyaddr, &value)
```

## Performance Characteristics

| Operation | Best Case | Worst Case | Notes |
|-----------|-----------|------------|-------|
| **Address Translation** | O(1) | O(1) | Direct page table lookup |
| **Memory Read (hit)** | O(1) | O(1) | Page already in RAM |
| **Memory Read (miss)** | O(n) | O(n) | Page fault + swapping |
| **Memory Write (hit)** | O(1) | O(1) | Page already in RAM |
| **Memory Write (miss)** | O(n) | O(n) | Page fault + swapping |
| **Allocation (space available)** | O(k) | O(k) | k = free regions to scan |
| **Allocation (expand VMA)** | O(p) | O(p+s) | p = pages, s = swap time |
| **Free** | O(1) | O(1) | Add to free list |
| **Find Victim** | O(1) | O(1) | FIFO tail removal |
| **Page Swap** | O(PAGE_SIZE) | O(PAGE_SIZE) | Copy operation |

## Configuration Examples

### Small System (1 MB RAM, 16 MB SWAP)

```c
// Configuration
RAM_SIZE = 1048576;      // 1 MB
SWAP_SIZE = 16777216;    // 16 MB
PAGE_SIZE = 256;         // 256 bytes

// Capacity
Total RAM pages: 4096
Total SWAP pages: 65536

// Typical usage
Active process pages: ~100-500
Swapped pages: ~1000-5000
```

### Example Process Memory Usage

```
Process with 3 variables:

Symbol Table:
  rgid=0: [0x0000-0x0400]  // 1KB array
  rgid=1: [0x0400-0x0410]  // 16B struct
  rgid=2: [0x0410-0x0810]  // 1KB buffer

Page Table (256B pages):
  PGN 0: FPN 5  (Present, Clean)
  PGN 1: FPN 7  (Present, Clean)
  PGN 2: FPN 12 (Present, Dirty)
  PGN 3: FPN 15 (Present, Dirty)
  PGN 4: SWAP 100 (Swapped)
  ...

FIFO List: [PGN 3] → [PGN 2] → [PGN 1] → [PGN 0]
```

## Error Handling

### Common Error Scenarios

1. **Page Fault with Full RAM and SWAP**
   - Error: Cannot allocate frame
   - Solution: Process must wait or be terminated

2. **Invalid Virtual Address**
   - Error: Address outside VMA range
   - Solution: Segmentation fault

3. **Invalid Region ID**
   - Error: Region not in symbol table
   - Solution: Return error code

4. **Allocation Failure**
   - Error: Cannot expand VMA (overlap)
   - Solution: Return NULL/error

5. **Swap Device Full**
   - Error: No free frames in SWAP
   - Solution: Process blocked or terminated

## Testing

### Unit Tests

```c
// Test 1: Allocate and write
liballoc(proc, 1024, 0);
libwrite(proc, 42, 0, 0);
libread(proc, 0, 0, &value);
assert(value == 42);

// Test 2: Page fault handling
// Allocate more than RAM size to force swapping
for (int i = 0; i < 10000; i++) {
  liballoc(proc, 256, i);
  libwrite(proc, i % 256, i, 0);
}

// Test 3: Free and reuse
liballoc(proc, 512, 0);
libfree(proc, 0);
liballoc(proc, 256, 1);  // Should reuse freed space
```

### Integration Tests

```bash
# Run paging tests
./os os_1_mlq_paging
./os os_1_singleCPU_mlq_paging

# Check output for:
# - Successful allocations
# - Page swapping messages
# - No memory errors
```

## Advantages

1. **Virtual Memory**: Processes see contiguous memory
2. **Memory Protection**: Each process isolated
3. **Efficient Swapping**: Only active pages in RAM
4. **Simple Management**: Fixed-size pages
5. **Flexible Allocation**: On-demand paging
6. **Page Replacement**: FIFO algorithm
7. **Dirty Tracking**: Only write back modified pages

## Future Enhancements

1. **Better Replacement**: LRU instead of FIFO
2. **Prefetching**: Load adjacent pages
3. **Copy-on-Write**: Share pages between processes
4. **Memory Compression**: Compress swapped pages
5. **NUMA Awareness**: Optimize for multi-socket systems
6. **Huge Pages**: 2MB/1GB pages for databases
7. **Page Clustering**: Group related pages

## Conclusion

The paging-based address translation system provides a complete virtual memory implementation with:
- ✅ Virtual-to-physical address translation
- ✅ Page fault handling
- ✅ Page swapping (RAM ↔ SWAP)
- ✅ FIFO page replacement
- ✅ Memory operations (ALLOC/FREE/READ/WRITE)
- ✅ Dirty page tracking
- ✅ Symbol table management

The system successfully integrates all memory management layers (libmem → mm-vm → mm → memphy) to provide transparent virtual memory to user processes.

