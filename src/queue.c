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
  /* TODO: put a new process to queue [q] */
  if (q == NULL || proc == NULL)
    return;
  if (q->size >= MAX_QUEUE_SIZE) 
    return;

  q->proc[q->size] = proc;
  q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
  /* TODO: return a pcb whose priority is the highest
  * in the queue [q] and remember to remove it from q
  * */
  struct pcb_t * proc = NULL;
  if(empty(q))
    return NULL;
  proc = q->proc[0];
  for(int i = 1; i < q->size; i++)
    q->proc[i - 1] = q->proc[i];

  q->size--;

  return proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
  /* TODO: remove a specific item from queue
  * */
  if (empty(q) || proc == NULL)
    return NULL;
  for (int i = 0; i < q->size; i++) 
  {
    if (q->proc[i]->pid == proc->pid) 
    {
      for(int j = i + 1; j < q->size; j++)
        q->proc[j - 1] = q->proc[j];

      q->size--;
      return proc;
    }
  }
  return NULL;
}