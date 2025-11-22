#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* Put a new process to queue [q] */
        if (q == NULL || proc == NULL)
                return;
        
        if (q->size >= MAX_QUEUE_SIZE)
        {
                printf("ERROR: Queue is full! Cannot enqueue process %d\n", proc->pid);
                return;
        }
        
        /* Add process to the end of queue (FIFO) */
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* Return a PCB whose priority is the highest in the queue [q]
         * and remove it from q.
         * 
         * In MLQ scheduling, all processes in the same queue have the same priority,
         * so we simply return the first process (FIFO within each priority level)
         */
        if (q == NULL || q->size == 0)
                return NULL;
        
        /* Get the first process */
        struct pcb_t *proc = q->proc[0];
        
        /* Shift all remaining processes forward */
        for (int i = 0; i < q->size - 1; i++)
        {
                q->proc[i] = q->proc[i + 1];
        }
        
        /* Decrease size */
        q->size--;
        
        return proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* Remove a specific item from queue */
        if (q == NULL || proc == NULL || q->size == 0)
                return NULL;
        
        /* Find the process in the queue */
        int found = -1;
        for (int i = 0; i < q->size; i++)
        {
                if (q->proc[i] == proc)
                {
                        found = i;
                        break;
                }
        }
        
        if (found == -1)
                return NULL;  /* Process not found */
        
        /* Remove the process by shifting remaining processes */
        for (int i = found; i < q->size - 1; i++)
        {
                q->proc[i] = q->proc[i + 1];
        }
        
        q->size--;
        
        return proc;
}