/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#ifndef OSCFG_H
#define OSCFG_H

/*
 * =====================================================================
 * OPERATING SYSTEM CONFIGURATION FILE
 * =====================================================================
 * This file controls all compile-time configuration options for the OS.
 * After modifying this file, always rebuild: make clean && make
 * =====================================================================
 */

/* ===== SCHEDULER CONFIGURATION ===== */

/* MLQ_SCHED: Enable Multi-Level Queue Scheduler
 * - Enables priority-based scheduling (similar to Linux)
 * - Adds 'prio' field to PCB structure
 * - Supports MAX_PRIO priority levels
 * DEFAULT: Enabled (recommended)
 */
#define MLQ_SCHED 1
#define MAX_PRIO 140    /* Maximum priority level (0-139) */

/* ===== MEMORY MANAGEMENT CONFIGURATION ===== */

/* MM_PAGING: Enable Paging-Based Memory Management
 * - Activates virtual memory with paging
 * - Enables page tables, RAM and SWAP devices
 * - Required for all memory management features
 * DEFAULT: Enabled (DO NOT DISABLE)
 */
#define MM_PAGING

/* MM_FIXED_MEMSZ: Fixed Memory Size (Backward Compatibility)
 * - When ENABLED: Uses fixed memory sizes (1MB RAM, 16MB SWAP)
 *                 Input file has 2-line header (old format)
 * - When DISABLED: Reads memory sizes from input file
 *                  Input file has 3-line header (new format)
 * 
 * Input File Format:
 * WITH MM_FIXED_MEMSZ (2 lines):
 *   [time_slice] [num_cpus] [num_processes]
 *   [time0] [path0] [priority0]
 *   ...
 * 
 * WITHOUT MM_FIXED_MEMSZ (3 lines):
 *   [time_slice] [num_cpus] [num_processes]
 *   [RAM_SIZE] [SWAP0_SIZE] [SWAP1_SIZE] [SWAP2_SIZE] [SWAP3_SIZE]
 *   [time0] [path0] [priority0]
 *   ...
 * 
 * DEFAULT: Disabled (use custom memory sizes)
 * CHANGE TO: Enabled for old input files
 */
//#define MM_FIXED_MEMSZ

/* MM64: 64-bit Memory Addressing Mode
 * - When ENABLED: Uses 64-bit addresses, 5-level page tables
 *                 Page size: 4KB, Address space: 128 PiB
 * - When DISABLED: Uses 32-bit addresses, single-level page table
 *                  Page size: 256B, Address space: 4 MB
 * 
 * Memory Structure:
 * WITH MM64:
 *   struct mm_struct {
 *     uint64_t *pgd, *p4d, *pud, *pmd, *pt;  // 5-level paging
 *   }
 * 
 * WITHOUT MM64:
 *   struct mm_struct {
 *     uint32_t *pgd;  // Single-level paging
 *   }
 * 
 * DEFAULT: Disabled (32-bit mode for simplicity)
 * CHANGE TO: Enabled for large memory simulations
 * 
 * NOTE: MM64 can be defined via command line (-DMM64=1) for 64-bit builds.
 *       Only undef if not already defined externally.
 */
#ifndef MM64
/* MM64 not defined externally, use default (disabled) */
//#define MM64 1
#endif

/* ===== DEBUG AND LOGGING CONFIGURATION ===== */

/* VMDBG: Virtual Memory Debug
 * - Prints detailed VMA operations
 * - Shows region allocation/deallocation
 * - Tracks page fault handling
 * WARNING: Very verbose output
 * DEFAULT: Disabled
 */
//#define VMDBG 1

/* MMDBG: Memory Management Debug
 * - Prints internal MM operations
 * - Shows frame allocation/deallocation
 * - Tracks swap operations
 * WARNING: Very verbose output
 * DEFAULT: Disabled
 */
//#define MMDBG 1

/* IODUMP: I/O Operation Dump
 * - Prints memory operation results
 * - Shows allocation addresses
 * - Displays memory content dumps
 * DEFAULT: Enabled (useful for debugging)
 */
#define IODUMP 1

/* PAGETBL_DUMP: Page Table Dump
 * - Prints page table entries after operations
 * - Shows PTE flags and mappings
 * - Useful for understanding paging
 * DEFAULT: Enabled (useful for debugging)
 */
#define PAGETBL_DUMP 1

/*
 * =====================================================================
 * CONFIGURATION PRESETS
 * =====================================================================
 * Uncomment ONE of the following sections to use a preset configuration
 * =====================================================================
 */

/* ===== PRESET 1: DEVELOPMENT/DEBUG MODE ===== */
/*
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
//#define MM_FIXED_MEMSZ
#define VMDBG 1
#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
//#define MM64 1
#undef MM64
*/

/* ===== PRESET 2: PRODUCTION/CLEAN MODE ===== */
/*
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
//#define MM_FIXED_MEMSZ
//#define VMDBG 1
//#define MMDBG 1
//#define IODUMP 1
//#define PAGETBL_DUMP 1
//#define MM64 1
#undef MM64
*/

/* ===== PRESET 3: BACKWARD COMPATIBLE MODE ===== */
/*
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
#define MM_FIXED_MEMSZ    // Use fixed memory
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
//#define MM64 1
#undef MM64
*/

/* ===== PRESET 4: 64-BIT LARGE MEMORY MODE ===== */
/*
#define MLQ_SCHED 1
#define MAX_PRIO 140
#define MM_PAGING
//#define MM_FIXED_MEMSZ
//#define VMDBG 1
//#define MMDBG 1
#define IODUMP 1
#define PAGETBL_DUMP 1
//#define MM64 1
#undef MM64
*/

/*
 * =====================================================================
 * END OF CONFIGURATION
 * =====================================================================
 * Remember to rebuild after changes: make clean && make
 * See CONFIGURATION_GUIDE.md for detailed documentation
 * =====================================================================
 */

#endif
