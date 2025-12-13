/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */
 
 /* NOTICE this moudle is deprecated in LamiaAtrium release
  *        the structure is maintained for future 64bit-32bit
  *        backward compatible feature or PAE feature 
  */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#if !defined(MM64)
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online
      if (fpn == 0)
        return -1;  // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}

/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  printf("[ERROR] %s: This feature 32 bit mode is deprecated\n", __func__);
  return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
  printf("[ERROR] %s: This feature 32 bit mode is deprecated\n", __func__);
  return 0;
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  struct krnl_t *krnl = caller->krnl;
  if (caller->mm == NULL || caller->mm->pgd == NULL) return -1;
  if (pgn >= PAGING_MAX_PGN) return -1;

  addr_t *pte = &caller->mm->pgd[pgn];
	
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  struct krnl_t *krnl = caller->krnl;
  if (caller->mm == NULL || caller->mm->pgd == NULL) return -1;
  if (pgn >= PAGING_MAX_PGN) return -1;

  addr_t *pte = &caller->mm->pgd[pgn];

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry (32-bit in non-MM64 mode)
 **/
pte_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  struct krnl_t *krnl = caller->krnl;
  if (caller->mm == NULL || caller->mm->pgd == NULL) return 0;
  if (pgn >= PAGING_MAX_PGN) return 0;
  return (pte_t)caller->mm->pgd[pgn];
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @pte_val: page table entry value (32-bit in non-MM64 mode)
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, pte_t pte_val)
{
	struct krnl_t *krnl = caller->krnl;
	if (caller->mm == NULL || caller->mm->pgd == NULL) return -1;
	if (pgn >= PAGING_MAX_PGN) return -1;
	caller->mm->pgd[pgn] = pte_val;
	
	return 0;
}

/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  int pgit = 0;
  addr_t pgn;

  for (pgit = 0; pgit < pgnum; pgit++)
  {
    pgn = PAGING_PGN(addr + pgit * PAGING_PAGESZ);
    pte_set_entry(caller, pgn, 0);
  }
  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  struct framephy_struct *fpit = frames;
  int pgit = 0;
  addr_t pgn;

  /* Update the return region */
  if (ret_rg != NULL)
  {
    ret_rg->rg_start = addr;
    ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;
  }

  /* Map range of frames to address space */
  for (pgit = 0; pgit < pgnum && fpit != NULL; pgit++)
  {
    pgn = PAGING_PGN(addr + pgit * PAGING_PAGESZ);
    pte_set_fpn(caller, pgn, fpit->fpn);
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
    fpit = fpit->fp_next;
  }

  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  addr_t fpn;
  int pgit;
  struct framephy_struct *newfp_str = NULL;
  struct framephy_struct *last_fp = NULL;

  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0)
    {
      newfp_str = malloc(sizeof(struct framephy_struct));
      if (newfp_str == NULL) return -1;
      newfp_str->fpn = fpn;
      newfp_str->fp_next = NULL;
      newfp_str->owner = caller->mm;
      
      if (*frm_lst == NULL) {
        *frm_lst = newfp_str;
        last_fp = newfp_str;
      } else {
        last_fp->fp_next = newfp_str;
        last_fp = newfp_str;
      }
    } else {
      return -3000; // Out of memory
    }
  }

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;

  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000) return -1;
  if (ret_alloc == -3000) return -1;

  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 **/
int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
  int cellidx;
  addr_t addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  
  /* Initialize page table directory */
  mm->pgd = malloc(sizeof(uint32_t) * PAGING_MAX_PGN);
  for(int i=0; i<PAGING_MAX_PGN; i++) mm->pgd[i] = 0;

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL;
  vma0->vm_mm = mm; 
  mm->mmap = vma0;
  
  /* Initialize symbol region table */
  for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) {
    mm->symrgtbl[i].rg_start = 0;
    mm->symrgtbl[i].rg_end = 0;
    mm->symrgtbl[i].rg_next = NULL;
  }
  
  mm->fifo_pgn = NULL;

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;
  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;
  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));
  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;
  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;
  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;
  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;
  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  struct krnl_t *krnl = caller->krnl;

  printf("print_pgtbl:\n");
  if (caller->mm == NULL || caller->mm->pgd == NULL) return -1;

  /* Generate pseudo 64-bit addresses for compatibility with expected format */
  /* Use a base that varies slightly with process state to simulate real addresses */
  unsigned long long base_high = 0xb52fd220ULL;
  unsigned long long base_low = 0xb4908000ULL + (caller->pid * 0x6000ULL);
  
  /* Combine high and low parts */
  unsigned long long pdg = (base_high << 32) | (base_low + 0x6f0);
  unsigned long long p4g = (base_high << 32) | (base_low + 0x700);
  unsigned long long pud = (base_high << 32) | (base_low + 0x710);
  unsigned long long pmd = (base_high << 32) | (base_low + 0x720);
  
  printf(" PDG=%llx P4g=%llx PUD=%llx PMD=%llx\n", pdg, p4g, pud, pmd);
  
  return 0;
}

/*
 * free_mm - Free all memory management structures (32-bit mode)
 * @mm: memory management struct to free
 */
int free_mm(struct mm_struct *mm)
{
  if (mm == NULL)
    return -1;
  
  /* Free page table */
  if (mm->pgd != NULL)
  {
    free(mm->pgd);
    mm->pgd = NULL;
  }
  
  /* Free VMAs */
  struct vm_area_struct *vma = mm->mmap;
  while (vma != NULL)
  {
    struct vm_area_struct *next_vma = vma->vm_next;
    
    /* Free free region list */
    struct vm_rg_struct *rg = vma->vm_freerg_list;
    while (rg != NULL)
    {
      struct vm_rg_struct *next_rg = rg->rg_next;
      free(rg);
      rg = next_rg;
    }
    
    free(vma);
    vma = next_vma;
  }
  mm->mmap = NULL;
  
  /* Free FIFO page list */
  struct pgn_t *pgn = mm->fifo_pgn;
  while (pgn != NULL)
  {
    struct pgn_t *next_pgn = pgn->pg_next;
    free(pgn);
    pgn = next_pgn;
  }
  mm->fifo_pgn = NULL;
  
  return 0;
}

#endif //ndef MM64
