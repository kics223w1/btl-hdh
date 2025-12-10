/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#if defined(MM64)

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
	/* Extract page direactories */
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */

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
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
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

  addr_t pgd_idx=0;
  addr_t p4d_idx=0;
  addr_t pud_idx=0;
  addr_t pmd_idx=0;
  addr_t pt_idx=0;
	
#ifdef MM64	
  /* Get page directory indices from page number */
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
  
  /* Navigate through page table hierarchy */
  if (krnl->mm->pgd == NULL)
    return -1;
  
  /* Access or allocate P4D */
  uint64_t **p4d_table = (uint64_t **)&krnl->mm->pgd[pgd_idx];
  if (*p4d_table == NULL)
  {
    *p4d_table = malloc(sizeof(uint64_t) * 512);
    if (*p4d_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*p4d_table)[i] = 0;
  }
  
  /* Access or allocate PUD */
  uint64_t **pud_table = (uint64_t **)&(*p4d_table)[p4d_idx];
  if (*pud_table == NULL)
  {
    *pud_table = malloc(sizeof(uint64_t) * 512);
    if (*pud_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pud_table)[i] = 0;
  }
  
  /* Access or allocate PMD */
  uint64_t **pmd_table = (uint64_t **)&(*pud_table)[pud_idx];
  if (*pmd_table == NULL)
  {
    *pmd_table = malloc(sizeof(uint64_t) * 512);
    if (*pmd_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pmd_table)[i] = 0;
  }
  
  /* Access or allocate PT */
  uint64_t **pt_table = (uint64_t **)&(*pmd_table)[pmd_idx];
  if (*pt_table == NULL)
  {
    *pt_table = malloc(sizeof(uint64_t) * 512);
    if (*pt_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pt_table)[i] = 0;
  }
  
  /* Get the actual PTE */
  addr_t *pte = (addr_t *)&(*pt_table)[pt_idx];
#else
  addr_t *pte = &krnl->mm->pgd[pgn];
#endif
	
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  struct krnl_t *krnl = caller->krnl;

  addr_t pgd_idx=0;
  addr_t p4d_idx=0;
  addr_t pud_idx=0;
  addr_t pmd_idx=0;
  addr_t pt_idx=0;
	
#ifdef MM64	
  /* Get page directory indices from page number */
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
  
  /* Navigate through page table hierarchy */
  if (krnl->mm->pgd == NULL)
    return -1;
  
  /* Access or allocate P4D */
  uint64_t **p4d_table = (uint64_t **)&krnl->mm->pgd[pgd_idx];
  if (*p4d_table == NULL)
  {
    *p4d_table = malloc(sizeof(uint64_t) * 512);
    if (*p4d_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*p4d_table)[i] = 0;
  }
  
  /* Access or allocate PUD */
  uint64_t **pud_table = (uint64_t **)&(*p4d_table)[p4d_idx];
  if (*pud_table == NULL)
  {
    *pud_table = malloc(sizeof(uint64_t) * 512);
    if (*pud_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pud_table)[i] = 0;
  }
  
  /* Access or allocate PMD */
  uint64_t **pmd_table = (uint64_t **)&(*pud_table)[pud_idx];
  if (*pmd_table == NULL)
  {
    *pmd_table = malloc(sizeof(uint64_t) * 512);
    if (*pmd_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pmd_table)[i] = 0;
  }
  
  /* Access or allocate PT */
  uint64_t **pt_table = (uint64_t **)&(*pmd_table)[pmd_idx];
  if (*pt_table == NULL)
  {
    *pt_table = malloc(sizeof(uint64_t) * 512);
    if (*pt_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pt_table)[i] = 0;
  }
  
  /* Get the actual PTE */
  addr_t *pte = (addr_t *)&(*pt_table)[pt_idx];
#else
  addr_t *pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry (64-bit in MM64 mode)
 **/
pte_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  struct krnl_t *krnl = caller->krnl;
  pte_t pte = 0;
  addr_t pgd_idx=0;
  addr_t p4d_idx=0;
  addr_t pud_idx=0;
  addr_t pmd_idx=0;
  addr_t pt_idx=0;
	
  /* Perform multi-level page mapping */
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
  
  /* Navigate through page table hierarchy */
  if (krnl->mm->pgd == NULL)
    return 0;
  
  /* Check P4D */
  uint64_t **p4d_table = (uint64_t **)&krnl->mm->pgd[pgd_idx];
  if (*p4d_table == NULL)
    return 0;
  
  /* Check PUD */
  uint64_t **pud_table = (uint64_t **)&(*p4d_table)[p4d_idx];
  if (*pud_table == NULL)
    return 0;
  
  /* Check PMD */
  uint64_t **pmd_table = (uint64_t **)&(*pud_table)[pud_idx];
  if (*pmd_table == NULL)
    return 0;
  
  /* Check PT */
  uint64_t **pt_table = (uint64_t **)&(*pmd_table)[pmd_idx];
  if (*pt_table == NULL)
    return 0;
  
  /* Get the actual PTE */
  pte = (pte_t)(*pt_table)[pt_idx];
	
  return pte;
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @pte_val: page table entry value (64-bit in MM64 mode)
 **/
int pte_set_entry(struct pcb_t *caller, addr_t pgn, pte_t pte_val)
{
  struct krnl_t *krnl = caller->krnl;
  
  addr_t pgd_idx=0;
  addr_t p4d_idx=0;
  addr_t pud_idx=0;
  addr_t pmd_idx=0;
  addr_t pt_idx=0;

  /* Get page directory indices */
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  if (krnl->mm->pgd == NULL)
    return -1;

  /* Access or allocate P4D */
  uint64_t **p4d_table = (uint64_t **)&krnl->mm->pgd[pgd_idx];
  if (*p4d_table == NULL)
  {
    *p4d_table = malloc(sizeof(uint64_t) * 512);
    if (*p4d_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*p4d_table)[i] = 0;
  }

  /* Access or allocate PUD */
  uint64_t **pud_table = (uint64_t **)&(*p4d_table)[p4d_idx];
  if (*pud_table == NULL)
  {
    *pud_table = malloc(sizeof(uint64_t) * 512);
    if (*pud_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pud_table)[i] = 0;
  }

  /* Access or allocate PMD */
  uint64_t **pmd_table = (uint64_t **)&(*pud_table)[pud_idx];
  if (*pmd_table == NULL)
  {
    *pmd_table = malloc(sizeof(uint64_t) * 512);
    if (*pmd_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pmd_table)[i] = 0;
  }

  /* Access or allocate PT */
  uint64_t **pt_table = (uint64_t **)&(*pmd_table)[pmd_idx];
  if (*pt_table == NULL)
  {
    *pt_table = malloc(sizeof(uint64_t) * 512);
    if (*pt_table == NULL) return -1;
    for (int i = 0; i < 512; i++) (*pt_table)[i] = 0;
  }

  /* Set the PTE value */
  (*pt_table)[pt_idx] = (uint64_t)pte_val;

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

  /* Initialize page table entries for the given range */
  for (pgit = 0; pgit < pgnum; pgit++)
  {
    /* Calculate page number from address */
    pgn = PAGING_PGN(addr + pgit * PAGING_PAGESZ);
    
    /* Initialize PTE with default values (not present, not swapped) */
    pte_t pte_val = 0;
    if (pte_set_entry(caller, pgn, pte_val) < 0)
    {
      return -1; /* Failed to set PTE */
    }
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

  /* Map range of frames to address space [addr to addr + pgnum*PAGING_PAGESZ] */
  for (pgit = 0; pgit < pgnum && fpit != NULL; pgit++)
  {
    /* Calculate page number from address */
    pgn = PAGING_PGN(addr + pgit * PAGING_PAGESZ);
    
    /* Set the page table entry to map page to frame */
    if (pte_set_fpn(caller, pgn, fpit->fpn) < 0)
    {
      return -1; /* Failed to set PTE */
    }
    
    /* Tracking for later page replacement activities (if needed)
     * Enqueue new usage page */
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
    
    /* Move to next frame */
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

  /* Allocate the requested number of frames */
  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    /* Try to get a free frame from RAM */
    if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0)
    {
      /* Successfully got a frame */
      newfp_str = malloc(sizeof(struct framephy_struct));
      if (newfp_str == NULL)
        return -1; /* Memory allocation failed */
      
      newfp_str->fpn = fpn;
      newfp_str->fp_next = NULL;
      newfp_str->owner = caller->krnl->mm;
      
      /* Add to the frame list */
      if (*frm_lst == NULL)
      {
        *frm_lst = newfp_str;
        last_fp = newfp_str;
      }
      else
      {
        last_fp->fp_next = newfp_str;
        last_fp = newfp_str;
      }
    }
    else
    {
      /* Failed to get frame - need swapping or out of memory */
      /* TODO: Implement page swapping mechanism */
      return -3000; /* Out of memory error code */
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
//  int pgnum = incpgnum;

  /*@bksysnet: author provides a feasible solution of getting frames
   *FATAL logic in here, wrong behaviour if we have not enough page
   *i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   *Don't try to perform that case in this simple work, it will result
   *in endless procedure of swap-off to get frame and we have not provide
   *duplicate control mechanism, keep it simple
   */
  // ret_alloc = alloc_pages_range(caller, pgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;

  /* Out of memory */
  if (ret_alloc == -3000)
  {
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
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
  
  if (vma0 == NULL)
    return -1;

  /* Initialize page table directory - allocate page tables */
  /* For 64-bit 5-level paging */
  mm->pgd = malloc(sizeof(uint64_t) * 512); // 512 entries for PGD
  mm->p4d = NULL; // Allocated on demand
  mm->pud = NULL; // Allocated on demand  
  mm->pmd = NULL; // Allocated on demand
  mm->pt = NULL;  // Allocated on demand
  
  if (mm->pgd == NULL)
  {
    free(vma0);
    return -1;
  }
  
  /* Initialize page table entries to 0 */
  for (int i = 0; i < 512; i++)
  {
    mm->pgd[i] = 0;
  }

  /* By default the owner comes with at least one vma (heap/data segment) */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  
  /* Initialize the free region list */
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  if (first_rg == NULL)
  {
    free(mm->pgd);
    free(vma0);
    return -1;
  }
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  /* Initialize VMA next pointer - initially no next VMA */
  vma0->vm_next = NULL;

  /* Point vma owner backward */
  vma0->vm_mm = mm; 

  /* Update mmap to point to the first VMA */
  mm->mmap = vma0;
  
  /* Initialize symbol region table */
  for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++)
  {
    mm->symrgtbl[i].rg_start = 0;
    mm->symrgtbl[i].rg_end = 0;
    mm->symrgtbl[i].rg_next = NULL;
  }
  
  /* Initialize FIFO page list for page replacement */
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
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
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
  addr_t pgn_start, pgn_end;
  addr_t pgit;
  struct krnl_t *krnl = caller->krnl;
  (void)krnl; /* Suppress unused variable warning */

  addr_t pgd_idx=0;
  addr_t p4d_idx=0;
  addr_t pud_idx=0;
  addr_t pmd_idx=0;
  addr_t pt_idx=0;

  /* Calculate start and end page numbers */
  pgn_start = PAGING_PGN(start);
  pgn_end = PAGING_PGN(end);

  printf("Page Table Dump (64-bit mode) [" FORMAT_ADDR " - " FORMAT_ADDR "]:\n", start, end);
  
  /* Traverse the page map and dump the page directory entries */
  for (pgit = pgn_start; pgit <= pgn_end; pgit++)
  {
    get_pd_from_pagenum(pgit, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);
    
    /* Get PTE value - now using pte_t (64-bit) */
    pte_t pte = pte_get_entry(caller, pgit);
    
    if (pte != 0)
    {
      /* Print page mapping information */
      printf("  PGN[" FORMAT_ADDR "] -> ", pgit);
      printf("PGD[" FORMAT_ADDR "] P4D[" FORMAT_ADDR "] PUD[" FORMAT_ADDR "] PMD[" FORMAT_ADDR "] PT[" FORMAT_ADDR "]", 
             pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx);
      
      /* Check if page is present */
      if (PAGING_PAGE_PRESENT(pte))
      {
        if (pte & PAGING_PTE_SWAPPED_MASK)
        {
          /* Page is swapped */
          addr_t swpoff = PAGING_PTE_SWP(pte);
          printf(" -> SWAPPED (offset: " FORMAT_ADDR ")\n", swpoff);
        }
        else
        {
          /* Page is in RAM */
          addr_t fpn = PAGING_PTE_FPN(pte);
          printf(" -> FPN[" FORMAT_ADDR "]\n", fpn);
        }
      }
      else
      {
        printf(" -> NOT PRESENT\n");
      }
    }
  }
  
  printf("\n");

  return 0;
}

/*
 * free_mm - Free all memory management structures
 * @mm: memory management struct to free
 * 
 * This function recursively frees all 5-level page tables and VMAs
 */
int free_mm(struct mm_struct *mm)
{
  int i, j, k, l;
  
  if (mm == NULL)
    return -1;
  
  /* Free 5-level page tables recursively */
  if (mm->pgd != NULL)
  {
    for (i = 0; i < 512; i++)
    {
      uint64_t **p4d_table = (uint64_t **)&mm->pgd[i];
      if (*p4d_table != NULL)
      {
        for (j = 0; j < 512; j++)
        {
          uint64_t **pud_table = (uint64_t **)&(*p4d_table)[j];
          if (*pud_table != NULL)
          {
            for (k = 0; k < 512; k++)
            {
              uint64_t **pmd_table = (uint64_t **)&(*pud_table)[k];
              if (*pmd_table != NULL)
              {
                for (l = 0; l < 512; l++)
                {
                  uint64_t **pt_table = (uint64_t **)&(*pmd_table)[l];
                  if (*pt_table != NULL)
                  {
                    free(*pt_table);
                    *pt_table = NULL;
                  }
                }
                free(*pmd_table);
                *pmd_table = NULL;
              }
            }
            free(*pud_table);
            *pud_table = NULL;
          }
        }
        free(*p4d_table);
        *p4d_table = NULL;
      }
    }
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

#endif  //def MM64
