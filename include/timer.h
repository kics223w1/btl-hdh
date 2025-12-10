#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdint.h>

struct timer_id_t {
	int done;
	int fsh;
	pthread_cond_t event_cond;
	pthread_mutex_t event_lock;
	pthread_cond_t timer_cond;
	pthread_mutex_t timer_lock;
};

void start_timer();

void stop_timer();

struct timer_id_t * attach_event();

void detach_event(struct timer_id_t * event);

void next_slot(struct timer_id_t* timer_id);

uint64_t current_time();

/* CPU ordering synchronization - ensures CPUs process in deterministic order */
void init_cpu_order(int num_cpus);
void wait_cpu_turn(int cpu_id);
void signal_next_cpu(int cpu_id);
void reset_cpu_order(void);
void mark_cpu_inactive(int cpu_id);

/* Barrier for synchronizing scheduling and execution phases */
void wait_scheduling_barrier(void);
void signal_scheduling_done(void);

#endif
