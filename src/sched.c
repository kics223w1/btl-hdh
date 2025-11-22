/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		slot[i] = MAX_PRIO - i; 
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	running_list.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;
	
	/* State variables to track current priority and remaining slots */
	static int curr_prio = 0;          /* Current priority level being served */
	static int curr_slot = 0;          /* Remaining slots for current priority */
	
	pthread_mutex_lock(&queue_lock);
	
	/* MLQ Policy Implementation:
	 * - Each priority level i gets slot[i] = MAX_PRIO - i time slots
	 * - Traverse through priority levels, giving each level its allocated slots
	 * - After exhausting slots for a level, move to next level
	 * - After scanning all levels, restart from priority 0
	 */
	
	/* Initialize current slot if starting fresh */
	if (curr_slot == 0)
	{
		/* Find next non-empty queue starting from curr_prio */
		int found = 0;
		for (int i = 0; i < MAX_PRIO; i++)
		{
			int check_prio = (curr_prio + i) % MAX_PRIO;
			if (!empty(&mlq_ready_queue[check_prio]))
			{
				curr_prio = check_prio;
				curr_slot = slot[curr_prio];  /* slot[i] = MAX_PRIO - i */
				found = 1;
				break;
			}
		}
		
		if (!found)
		{
			/* All queues are empty */
			pthread_mutex_unlock(&queue_lock);
			return NULL;
		}
	}
	
	/* Get process from current priority queue */
	proc = dequeue(&mlq_ready_queue[curr_prio]);
	
	if (proc != NULL)
	{
		/* Successfully got a process */
		curr_slot--;  /* Decrease remaining slots for this priority */
		
		/* Add to running list for tracking */
		enqueue(&running_list, proc);
	}
	else
	{
		/* Current queue became empty, reset slots to move to next priority */
		curr_slot = 0;
		
		/* Try again recursively to find next available process */
		pthread_mutex_unlock(&queue_lock);
		return get_mlq_proc();
	}
	
	/* Check if we've exhausted slots for current priority */
	if (curr_slot == 0)
	{
		/* Move to next priority level */
		curr_prio = (curr_prio + 1) % MAX_PRIO;
	}
	
	pthread_mutex_unlock(&queue_lock);
	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* Put the process back to its priority ready queue
	 * This is called when a process's time slice expires
	 * The process is re-queued to the same priority level (no feedback)
	 */
	
	pthread_mutex_lock(&queue_lock);
	
	/* Remove from running list if present */
	purgequeue(&running_list, proc);
	
	/* Add back to appropriate priority queue */
	enqueue(&mlq_ready_queue[proc->prio], proc);
	
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* Add a newly loaded process to its appropriate priority queue
	 * This is called by the loader when a new process is created
	 * The process is placed in the queue matching its priority level
	 */
       
	pthread_mutex_lock(&queue_lock);
	
	/* Add process to the queue matching its priority */
	enqueue(&mlq_ready_queue[proc->prio], proc);
	
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);
	/*TODO: get a process from [ready_queue].
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_unlock(&queue_lock);

	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif


