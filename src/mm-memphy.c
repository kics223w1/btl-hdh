/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, addr_t offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while (numstep < offset && numstep < mp->maxsz)
   {
      /* Traverse sequentially */
      mp->cursor = (mp->cursor + 1) % mp->maxsz;
      numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE)mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (mp->rdmflg)
   {
      if (addr >= mp->maxsz)
         return -1; /* Out of bounds */
      *value = mp->storage[addr];
   }
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, addr_t addr, BYTE value)
{

   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct *mp, addr_t addr, BYTE data)
{
   if (mp == NULL)
      return -1;

   if (mp->rdmflg)
   {
      if (addr >= mp->maxsz)
         return -1; /* Out of bounds */
      mp->storage[addr] = data;
   }
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   /* This setting come with fixed constant PAGESZ */
   int numfp = mp->maxsz / pagesz;
   struct framephy_struct *newfst, *fst;
   int iter = 0;

   if (numfp <= 0)
      return -1;

   /* Init head of free framephy list */
   fst = malloc(sizeof(struct framephy_struct));
   fst->fpn = iter;
   mp->free_fp_list = fst;

   /* We have list with first element, fill in the rest num-1 element member*/
   for (iter = 1; iter < numfp; iter++)
   {
      newfst = malloc(sizeof(struct framephy_struct));
      newfst->fpn = iter;
      newfst->fp_next = NULL;
      fst->fp_next = newfst;
      fst = newfst;
   }

   return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn)
{
   struct framephy_struct *fp = mp->free_fp_list;

   if (fp == NULL)
      return -1;

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   free(fp);

   return 0;
}

int MEMPHY_dump(struct memphy_struct *mp)
{
  /* Dump memphy content mp->storage for tracing the memory content */
  if (mp == NULL)
  {
    printf("MEMPHY_dump: NULL memory physical device\n");
    return -1;
  }

  printf("=== MEMPHY DUMP ===\n");
  printf("Max Size: %d bytes\n", mp->maxsz);
  printf("Access Mode: %s\n", mp->rdmflg ? "Random" : "Sequential");
  
  if (!mp->rdmflg)
    printf("Cursor Position: %d\n", mp->cursor);
  
  /* Dump storage content in hexadecimal format */
  printf("\nStorage Content (first 256 bytes):\n");
  int dump_size = (mp->maxsz < 256) ? mp->maxsz : 256;
  
  for (int i = 0; i < dump_size; i++)
  {
    if (i % 16 == 0)
      printf("%04x: ", i);
    
    printf("%02x ", (unsigned char)mp->storage[i]);
    
    if ((i + 1) % 16 == 0)
      printf("\n");
  }
  
  if (dump_size % 16 != 0)
    printf("\n");
  
  /* Dump free frame list */
  printf("\nFree Frame List:\n");
  struct framephy_struct *fp = mp->free_fp_list;
  int free_count = 0;
  
  while (fp != NULL && free_count < 20) // Limit to first 20 frames
  {
    printf("  FPN: " FORMAT_ADDR, fp->fpn);
    if (free_count % 5 == 4)
      printf("\n");
    else
      printf(", ");
    fp = fp->fp_next;
    free_count++;
  }
  
  if (free_count > 0 && free_count % 5 != 0)
    printf("\n");
  
  if (fp != NULL)
    printf("  ... (more frames)\n");
  
  printf("Total Free Frames Shown: %d\n", free_count);
  
  /* Dump used frame list */
  printf("\nUsed Frame List:\n");
  struct framephy_struct *ufp = mp->used_fp_list;
  int used_count = 0;
  
  while (ufp != NULL && used_count < 20) // Limit to first 20 frames
  {
    printf("  FPN: " FORMAT_ADDR, ufp->fpn);
    if (ufp->owner != NULL)
      printf(" (owner: %p)", (void*)ufp->owner);
    if (used_count % 5 == 4)
      printf("\n");
    else
      printf(", ");
    ufp = ufp->fp_next;
    used_count++;
  }
  
  if (used_count > 0 && used_count % 5 != 0)
    printf("\n");
  
  if (ufp != NULL)
    printf("  ... (more frames)\n");
  
  printf("Total Used Frames Shown: %d\n", used_count);
  
  printf("===================\n\n");
  
  return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn)
{
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   newnode->owner = NULL;
   mp->free_fp_list = newnode;

   return 0;
}

/*
 * MEMPHY_get_usedfp - Move a frame from free list to used list
 * @mp: memphy struct
 * @fpn: frame page number
 * @owner: owner of the frame
 */
int MEMPHY_get_usedfp(struct memphy_struct *mp, addr_t fpn, struct mm_struct *owner)
{
   /* Get frame from free list */
   addr_t allocated_fpn;
   if (MEMPHY_get_freefp(mp, &allocated_fpn) < 0)
      return -1;
   
   /* Create node for used list */
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));
   if (newnode == NULL)
      return -1;
   
   newnode->fpn = allocated_fpn;
   newnode->owner = owner;
   newnode->fp_next = mp->used_fp_list;
   mp->used_fp_list = newnode;
   
   return 0;
}

/*
 * MEMPHY_put_usedfp - Add a frame to used list
 * @mp: memphy struct
 * @fpn: frame page number
 * @owner: owner of the frame
 */
int MEMPHY_put_usedfp(struct memphy_struct *mp, addr_t fpn, struct mm_struct *owner)
{
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));
   
   if (newnode == NULL)
      return -1;
   
   newnode->fpn = fpn;
   newnode->owner = owner;
   newnode->fp_next = mp->used_fp_list;
   mp->used_fp_list = newnode;
   
   return 0;
}

/*
 * MEMPHY_remove_usedfp - Remove a frame from used list
 * @mp: memphy struct
 * @fpn: frame page number to remove
 */
int MEMPHY_remove_usedfp(struct memphy_struct *mp, addr_t fpn)
{
   struct framephy_struct *prev = NULL;
   struct framephy_struct *curr = mp->used_fp_list;
   
   /* Search for the frame in used list */
   while (curr != NULL)
   {
      if (curr->fpn == fpn)
      {
         /* Found it, remove from list */
         if (prev == NULL)
            mp->used_fp_list = curr->fp_next;
         else
            prev->fp_next = curr->fp_next;
         
         free(curr);
         return 0;
      }
      prev = curr;
      curr = curr->fp_next;
   }
   
   return -1; /* Frame not found */
}

/*
 * MEMPHY_free_usedfp - Move a frame from used list back to free list
 * @mp: memphy struct
 * @fpn: frame page number to free
 */
int MEMPHY_free_usedfp(struct memphy_struct *mp, addr_t fpn)
{
   /* Remove from used list */
   if (MEMPHY_remove_usedfp(mp, fpn) < 0)
      return -1;
   
   /* Add back to free list */
   return MEMPHY_put_freefp(mp, fpn);
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, addr_t max_size, int randomflg)
{
   mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
   mp->maxsz = max_size;
   memset(mp->storage, 0, max_size * sizeof(BYTE));

   /* Initialize used frame list to NULL */
   mp->used_fp_list = NULL;

   if (MEMPHY_format(mp, PAGING_PAGESZ) < 0)
     mp->free_fp_list = NULL;

   mp->rdmflg = (randomflg != 0) ? 1 : 0;

   if (!mp->rdmflg) /* Not Random access device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

/*
 * MEMPHY_get_frame_count - Count number of frames in a list
 * @fp_list: frame list to count
 */
int MEMPHY_get_frame_count(struct framephy_struct *fp_list)
{
   int count = 0;
   struct framephy_struct *fp = fp_list;
   
   while (fp != NULL)
   {
      count++;
      fp = fp->fp_next;
   }
   
   return count;
}

/*
 * MEMPHY_get_stats - Get memory statistics
 * @mp: memphy struct
 * @free_frames: output - number of free frames
 * @used_frames: output - number of used frames
 * @total_frames: output - total number of frames
 */
int MEMPHY_get_stats(struct memphy_struct *mp, int *free_frames, int *used_frames, int *total_frames)
{
   if (mp == NULL)
      return -1;
   
   *free_frames = MEMPHY_get_frame_count(mp->free_fp_list);
   *used_frames = MEMPHY_get_frame_count(mp->used_fp_list);
   *total_frames = mp->maxsz / PAGING_PAGESZ;
   
   return 0;
}

/*
 * MEMPHY_print_stats - Print memory statistics
 * @mp: memphy struct
 * @name: name of the memory device (e.g., "RAM", "SWAP0")
 */
int MEMPHY_print_stats(struct memphy_struct *mp, const char *name)
{
   if (mp == NULL)
      return -1;
   
   int free_frames, used_frames, total_frames;
   MEMPHY_get_stats(mp, &free_frames, &used_frames, &total_frames);
   
   printf("=== %s Statistics ===\n", name);
   printf("Total Size: %d bytes (%d KB)\n", mp->maxsz, mp->maxsz / 1024);
   printf("Total Frames: %d\n", total_frames);
   printf("Free Frames: %d (%.1f%%)\n", free_frames, 
          (float)free_frames * 100.0 / total_frames);
   printf("Used Frames: %d (%.1f%%)\n", used_frames,
          (float)used_frames * 100.0 / total_frames);
   printf("Access Mode: %s\n", mp->rdmflg ? "Random" : "Sequential");
   printf("======================\n");
   
   return 0;
}

/*
 * MEMPHY_find_frame - Find a frame in used list by FPN
 * @mp: memphy struct
 * @fpn: frame page number to find
 * @owner: output - owner of the frame (if found)
 */
int MEMPHY_find_frame(struct memphy_struct *mp, addr_t fpn, struct mm_struct **owner)
{
   struct framephy_struct *fp = mp->used_fp_list;
   
   while (fp != NULL)
   {
      if (fp->fpn == fpn)
      {
         if (owner != NULL)
            *owner = fp->owner;
         return 0; /* Found */
      }
      fp = fp->fp_next;
   }
   
   return -1; /* Not found */
}

/*
 * MEMPHY_is_frame_free - Check if a frame is in free list
 * @mp: memphy struct
 * @fpn: frame page number to check
 */
int MEMPHY_is_frame_free(struct memphy_struct *mp, addr_t fpn)
{
   struct framephy_struct *fp = mp->free_fp_list;
   
   while (fp != NULL)
   {
      if (fp->fpn == fpn)
         return 1; /* Frame is free */
      fp = fp->fp_next;
   }
   
   return 0; /* Frame is not free */
}

/*
 * MEMPHY_validate - Validate memory physical device
 * @mp: memphy struct
 */
int MEMPHY_validate(struct memphy_struct *mp)
{
   if (mp == NULL)
   {
      printf("MEMPHY_validate: NULL memphy struct\n");
      return -1;
   }
   
   if (mp->storage == NULL)
   {
      printf("MEMPHY_validate: NULL storage\n");
      return -1;
   }
   
   if (mp->maxsz <= 0)
   {
      printf("MEMPHY_validate: Invalid max size: %d\n", mp->maxsz);
      return -1;
   }
   
   /* Count frames and check for duplicates */
   int free_count = MEMPHY_get_frame_count(mp->free_fp_list);
   int used_count = MEMPHY_get_frame_count(mp->used_fp_list);
   int total_frames = mp->maxsz / PAGING_PAGESZ;
   
   if (free_count + used_count > total_frames)
   {
      printf("MEMPHY_validate: Frame count mismatch (free: %d, used: %d, total: %d)\n",
             free_count, used_count, total_frames);
      return -1;
   }
   
   return 0;
}

/*
 * MEMPHY_cleanup - Free all allocated memory in memphy
 * @mp: memphy struct
 */
int MEMPHY_cleanup(struct memphy_struct *mp)
{
   if (mp == NULL)
      return -1;
   
   /* Free storage */
   if (mp->storage != NULL)
   {
      free(mp->storage);
      mp->storage = NULL;
   }
   
   /* Free all nodes in free list */
   struct framephy_struct *fp = mp->free_fp_list;
   while (fp != NULL)
   {
      struct framephy_struct *next = fp->fp_next;
      free(fp);
      fp = next;
   }
   mp->free_fp_list = NULL;
   
   /* Free all nodes in used list */
   fp = mp->used_fp_list;
   while (fp != NULL)
   {
      struct framephy_struct *next = fp->fp_next;
      free(fp);
      fp = next;
   }
   mp->used_fp_list = NULL;
   
   return 0;
}

// #endif
