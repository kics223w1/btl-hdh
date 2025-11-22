/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn , addr_t swpfpn)
{
    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->kernl->mm, vmaid);

  //newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;
  /* END TODO */

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  //struct vm_area_struct *vma = caller->krnl->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend)
  {
    return -1;
  }

  struct vm_area_struct *vma = caller->krnl->mm->mmap;
  if (vma == NULL)
  {
    return -1;
  }

  /* TODO validate the planned memory area is not overlapped */

  struct vm_area_struct *cur_area = get_vma_by_num(caller->krnl->mm, vmaid);
  if (cur_area == NULL)
  {
    return -1;
  }

  while (vma != NULL)
  {
    if (vma != cur_area && OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end))
    {
      return -1;
    }
    vma = vma->vm_next;
  }
  /* End TODO*/

  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  
  if (cur_vma == NULL)
  {
    free(newrg);
    return -1;
  }

  /* Align the size to page size */
  addr_t inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;

  /* Save old end for mapping */
  addr_t old_sbrk = cur_vma->sbrk;
  addr_t old_end = cur_vma->vm_end;
  
  /* Update the VMA end and sbrk */
  cur_vma->vm_end += inc_amt;
  cur_vma->sbrk += inc_sz; // sbrk increases by actual requested size
  
  /* Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, cur_vma->vm_start, cur_vma->vm_end) < 0)
  {
    /* Rollback changes */
    cur_vma->vm_end = old_end;
    cur_vma->sbrk = old_sbrk;
    free(newrg);
    return -1; /* Overlap and failed allocation */
  }

  /* Map the memory to MEMRAM */
  if (vm_map_ram(caller, old_end, cur_vma->vm_end, 
                   old_end, incnumpage, newrg) < 0)
  {
    /* Rollback changes */
    cur_vma->vm_end = old_end;
    cur_vma->sbrk = old_sbrk;
    free(newrg);
    return -1; /* Map failed */
  }

  return 0;
}

/*create_vm_area - create a new vm area
 *@vmaid: ID of the new vm area
 *@vm_start: start address of the vm area
 *@vm_end: end address of the vm area
 *
 *Return: pointer to the newly created vm area
 */
struct vm_area_struct *create_vm_area(int vmaid, addr_t vm_start, addr_t vm_end)
{
  struct vm_area_struct *new_vma = malloc(sizeof(struct vm_area_struct));
  
  if (new_vma == NULL)
    return NULL;
  
  new_vma->vm_id = vmaid;
  new_vma->vm_start = vm_start;
  new_vma->vm_end = vm_end;
  new_vma->sbrk = vm_start;
  new_vma->vm_mm = NULL;
  new_vma->vm_freerg_list = NULL;
  new_vma->vm_next = NULL;
  
  /* Initialize with a free region covering the entire area */
  struct vm_rg_struct *init_rg = malloc(sizeof(struct vm_rg_struct));
  if (init_rg != NULL)
  {
    init_rg->rg_start = vm_start;
    init_rg->rg_end = vm_start; // Initially, no usable area
    init_rg->rg_next = NULL;
    new_vma->vm_freerg_list = init_rg;
  }
  
  return new_vma;
}

/*add_vm_area - add a new vm area to the mm struct
 *@mm: memory management struct
 *@new_vma: the vm area to add
 *
 *Return: 0 on success, -1 on failure
 */
int add_vm_area(struct mm_struct *mm, struct vm_area_struct *new_vma)
{
  if (mm == NULL || new_vma == NULL)
    return -1;
  
  /* Check for overlaps with existing VMAs */
  struct vm_area_struct *cur = mm->mmap;
  while (cur != NULL)
  {
    if (OVERLAP(cur->vm_start, cur->vm_end, new_vma->vm_start, new_vma->vm_end))
      return -1; /* Overlap detected */
    cur = cur->vm_next;
  }
  
  /* Set the mm pointer */
  new_vma->vm_mm = mm;
  
  /* Insert at the beginning or in sorted order by vm_id */
  if (mm->mmap == NULL || mm->mmap->vm_id > new_vma->vm_id)
  {
    new_vma->vm_next = mm->mmap;
    mm->mmap = new_vma;
  }
  else
  {
    struct vm_area_struct *prev = mm->mmap;
    cur = mm->mmap->vm_next;
    
    while (cur != NULL && cur->vm_id < new_vma->vm_id)
    {
      prev = cur;
      cur = cur->vm_next;
    }
    
    new_vma->vm_next = cur;
    prev->vm_next = new_vma;
  }
  
  return 0;
}

/*remove_vm_area - remove a vm area from the mm struct
 *@mm: memory management struct
 *@vmaid: ID of the vm area to remove
 *
 *Return: 0 on success, -1 on failure
 */
int remove_vm_area(struct mm_struct *mm, int vmaid)
{
  if (mm == NULL || mm->mmap == NULL)
    return -1;
  
  struct vm_area_struct *prev = NULL;
  struct vm_area_struct *cur = mm->mmap;
  
  /* Find the VMA to remove */
  while (cur != NULL && cur->vm_id != vmaid)
  {
    prev = cur;
    cur = cur->vm_next;
  }
  
  if (cur == NULL)
    return -1; /* VMA not found */
  
  /* Remove from list */
  if (prev == NULL)
    mm->mmap = cur->vm_next;
  else
    prev->vm_next = cur->vm_next;
  
  /* Free the VMA and its free region list */
  struct vm_rg_struct *rg = cur->vm_freerg_list;
  while (rg != NULL)
  {
    struct vm_rg_struct *next_rg = rg->rg_next;
    free(rg);
    rg = next_rg;
  }
  
  free(cur);
  return 0;
}

/*merge_vm_areas - merge two adjacent vm areas
 *@vma1: first vm area
 *@vma2: second vm area
 *
 *Return: 0 on success, -1 on failure
 */
int merge_vm_areas(struct vm_area_struct *vma1, struct vm_area_struct *vma2)
{
  if (vma1 == NULL || vma2 == NULL)
    return -1;
  
  /* Check if areas are adjacent */
  if (vma1->vm_end != vma2->vm_start && vma2->vm_end != vma1->vm_start)
    return -1; /* Not adjacent */
  
  /* Ensure vma1 comes before vma2 */
  if (vma1->vm_start > vma2->vm_start)
  {
    struct vm_area_struct *temp = vma1;
    vma1 = vma2;
    vma2 = temp;
  }
  
  /* Merge vma2 into vma1 */
  vma1->vm_end = vma2->vm_end;
  vma1->sbrk = (vma1->sbrk > vma2->sbrk) ? vma1->sbrk : vma2->sbrk;
  
  /* Merge free region lists */
  if (vma1->vm_freerg_list != NULL)
  {
    struct vm_rg_struct *last_rg = vma1->vm_freerg_list;
    while (last_rg->rg_next != NULL)
      last_rg = last_rg->rg_next;
    last_rg->rg_next = vma2->vm_freerg_list;
  }
  else
  {
    vma1->vm_freerg_list = vma2->vm_freerg_list;
  }
  
  /* Update the next pointer */
  vma1->vm_next = vma2->vm_next;
  
  /* Don't free vma2's regions, as they're now part of vma1 */
  vma2->vm_freerg_list = NULL;
  
  return 0;
}

/*split_vm_area - split a vm area at a given address
 *@vma: vm area to split
 *@split_addr: address at which to split
 *@new_vma: pointer to store the new vm area
 *
 *Return: 0 on success, -1 on failure
 */
int split_vm_area(struct vm_area_struct *vma, addr_t split_addr, struct vm_area_struct **new_vma)
{
  if (vma == NULL || new_vma == NULL)
    return -1;
  
  /* Check if split address is within the VMA */
  if (split_addr <= vma->vm_start || split_addr >= vma->vm_end)
    return -1;
  
  /* Create new VMA for the upper part */
  *new_vma = malloc(sizeof(struct vm_area_struct));
  if (*new_vma == NULL)
    return -1;
  
  (*new_vma)->vm_id = vma->vm_id + 1; /* New ID for split VMA */
  (*new_vma)->vm_start = split_addr;
  (*new_vma)->vm_end = vma->vm_end;
  (*new_vma)->sbrk = (vma->sbrk > split_addr) ? vma->sbrk : split_addr;
  (*new_vma)->vm_mm = vma->vm_mm;
  (*new_vma)->vm_next = vma->vm_next;
  (*new_vma)->vm_freerg_list = NULL;
  
  /* Update original VMA */
  vma->vm_end = split_addr;
  if (vma->sbrk > split_addr)
    vma->sbrk = split_addr;
  vma->vm_next = *new_vma;
  
  /* Split the free region list */
  struct vm_rg_struct *rg = vma->vm_freerg_list;
  struct vm_rg_struct *prev_rg = NULL;
  
  while (rg != NULL)
  {
    if (rg->rg_start >= split_addr)
    {
      /* Move this region to new VMA */
      if (prev_rg == NULL)
        vma->vm_freerg_list = NULL;
      else
        prev_rg->rg_next = NULL;
      
      (*new_vma)->vm_freerg_list = rg;
      break;
    }
    else if (rg->rg_end > split_addr)
    {
      /* Region spans the split, need to split it */
      struct vm_rg_struct *new_rg = malloc(sizeof(struct vm_rg_struct));
      if (new_rg != NULL)
      {
        new_rg->rg_start = split_addr;
        new_rg->rg_end = rg->rg_end;
        new_rg->rg_next = rg->rg_next;
        
        rg->rg_end = split_addr;
        (*new_vma)->vm_freerg_list = new_rg;
      }
      break;
    }
    
    prev_rg = rg;
    rg = rg->rg_next;
  }

  return 0;
}

// #endif
