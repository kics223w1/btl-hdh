
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>

static pthread_t _timer;

struct timer_id_container_t {
	struct timer_id_t id;
	struct timer_id_container_t * next;
};

static struct timer_id_container_t * dev_list = NULL;

static uint64_t _time;

static int timer_started = 0;
static int timer_stop = 0;

/* CPU ordering synchronization variables */
static int _num_cpus = 0;
static int _current_cpu_turn = -1;  /* -1 means loader's turn, then highest CPU first */
static int _cpu_active[64];  /* Track which CPUs are still active */
static pthread_mutex_t _cpu_order_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _cpu_order_cond = PTHREAD_COND_INITIALIZER;

/* Barrier for scheduling/execution synchronization */
static int _scheduling_done_count = 0;
static int _scheduling_barrier_released = 0;
static pthread_mutex_t _barrier_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _barrier_cond = PTHREAD_COND_INITIALIZER;


static void * timer_routine(void * args) {
	while (!timer_stop) {
		printf("Time slot %3llu\n", current_time());
		int fsh = 0;
		int event = 0;
		/* Wait for all devices have done the job in current
		 * time slot */
		struct timer_id_container_t * temp;
		for (temp = dev_list; temp != NULL; temp = temp->next) {
			pthread_mutex_lock(&temp->id.event_lock);
			while (!temp->id.done && !temp->id.fsh) {
				pthread_cond_wait(
					&temp->id.event_cond,
					&temp->id.event_lock
				);
			}
			if (temp->id.fsh) {
				fsh++;
			}
			event++;
			pthread_mutex_unlock(&temp->id.event_lock);
		}

		/* Increase the time slot */
		_time++;
		
		/* Reset CPU order for the new time slot */
		reset_cpu_order();
		
		/* Let devices continue their job */
		for (temp = dev_list; temp != NULL; temp = temp->next) {
			pthread_mutex_lock(&temp->id.timer_lock);
			temp->id.done = 0;
			pthread_cond_signal(&temp->id.timer_cond);
			pthread_mutex_unlock(&temp->id.timer_lock);
		}
		if (fsh == event) {
			break;
		}
	}
	pthread_exit(args);
}

void next_slot(struct timer_id_t * timer_id) {
	/* Tell to timer that we have done our job in current slot */
	pthread_mutex_lock(&timer_id->event_lock);
	timer_id->done = 1;
	pthread_cond_signal(&timer_id->event_cond);
	pthread_mutex_unlock(&timer_id->event_lock);

	/* Wait for going to next slot */
	pthread_mutex_lock(&timer_id->timer_lock);
	while (timer_id->done) {
		pthread_cond_wait(
			&timer_id->timer_cond,
			&timer_id->timer_lock
		);
	}
	pthread_mutex_unlock(&timer_id->timer_lock);
}

uint64_t current_time() {
	return _time;
}

void start_timer() {
	timer_started = 1;
	pthread_create(&_timer, NULL, timer_routine, NULL);
}

void detach_event(struct timer_id_t * event) {
	pthread_mutex_lock(&event->event_lock);
	event->fsh = 1;
	pthread_cond_signal(&event->event_cond);
	pthread_mutex_unlock(&event->event_lock);
}

struct timer_id_t * attach_event() {
	if (timer_started) {
		return NULL;
	}else{
		struct timer_id_container_t * container =
			(struct timer_id_container_t*)malloc(
				sizeof(struct timer_id_container_t)		
			);
		container->id.done = 0;
		container->id.fsh = 0;
		pthread_cond_init(&container->id.event_cond, NULL);
		pthread_mutex_init(&container->id.event_lock, NULL);
		pthread_cond_init(&container->id.timer_cond, NULL);
		pthread_mutex_init(&container->id.timer_lock, NULL);
		if (dev_list == NULL) {
			dev_list = container;
			dev_list->next = NULL;
		}else{
			container->next = dev_list;
			dev_list = container;
		}
		return &(container->id);
	}
}

void stop_timer() {
	timer_stop = 1;
	pthread_join(_timer, NULL);
	while (dev_list != NULL) {
		struct timer_id_container_t * temp = dev_list;
		dev_list = dev_list->next;
		pthread_cond_destroy(&temp->id.event_cond);
		pthread_mutex_destroy(&temp->id.event_lock);
		pthread_cond_destroy(&temp->id.timer_cond);
		pthread_mutex_destroy(&temp->id.timer_lock);
		free(temp);
	}
}

/* Initialize CPU ordering with the number of CPUs */
void init_cpu_order(int num_cpus) {
	_num_cpus = num_cpus;
	_current_cpu_turn = num_cpus - 1;  /* Start with highest CPU ID */
	for (int i = 0; i < num_cpus && i < 64; i++) {
		_cpu_active[i] = 1;  /* All CPUs start as active */
	}
}

/* Find the next active CPU (going from current down to 0) */
static int find_next_active_cpu(int from) {
	for (int i = from; i >= 0; i--) {
		if (_cpu_active[i]) {
			return i;
		}
	}
	return -1;  /* No active CPU found, signal end of cycle */
}

/* Find the highest active CPU */
static int find_highest_active_cpu(void) {
	for (int i = _num_cpus - 1; i >= 0; i--) {
		if (_cpu_active[i]) {
			return i;
		}
	}
	return -1;  /* No active CPUs */
}

/* Wait for this CPU's turn to process 
 * cpu_id: 0 to num_cpus-1 for CPUs, -1 for loader
 * Order: highest CPU first (num_cpus-1), then decreasing to 0
 */
void wait_cpu_turn(int cpu_id) {
	pthread_mutex_lock(&_cpu_order_lock);
	while (_current_cpu_turn != cpu_id) {
		pthread_cond_wait(&_cpu_order_cond, &_cpu_order_lock);
	}
	pthread_mutex_unlock(&_cpu_order_lock);
}

/* Mark a CPU as inactive (when it exits) */
void mark_cpu_inactive(int cpu_id) {
	pthread_mutex_lock(&_cpu_order_lock);
	if (cpu_id >= 0 && cpu_id < 64) {
		_cpu_active[cpu_id] = 0;
	}
	pthread_mutex_unlock(&_cpu_order_lock);
}

/* Signal that this CPU/loader is done, let next one proceed */
void signal_next_cpu(int cpu_id) {
	pthread_mutex_lock(&_cpu_order_lock);
	if (cpu_id == -1) {
		/* Loader is done, reset for next time slot */
		int highest = find_highest_active_cpu();
		_current_cpu_turn = (highest >= 0) ? highest : -1;
	} else if (cpu_id == 0) {
		/* CPU 0 is done, loader's turn */
		_current_cpu_turn = -1;
	} else {
		/* Find next active CPU (lower ID) */
		int next = find_next_active_cpu(cpu_id - 1);
		if (next >= 0) {
			_current_cpu_turn = next;
		} else {
			/* No more active CPUs, loader's turn */
			_current_cpu_turn = -1;
		}
	}
	pthread_cond_broadcast(&_cpu_order_cond);
	pthread_mutex_unlock(&_cpu_order_lock);
}

/* Reset CPU order for next time slot */
void reset_cpu_order(void) {
	pthread_mutex_lock(&_cpu_order_lock);
	int highest = find_highest_active_cpu();
	_current_cpu_turn = (highest >= 0) ? highest : -1;
	pthread_cond_broadcast(&_cpu_order_cond);
	pthread_mutex_unlock(&_cpu_order_lock);
	
	/* Reset barrier for new time slot */
	pthread_mutex_lock(&_barrier_lock);
	_scheduling_done_count = 0;
	_scheduling_barrier_released = 0;
	pthread_mutex_unlock(&_barrier_lock);
}

/* Count active participants (CPUs + loader) */
static int count_active_participants(void) {
	int count = 1; /* loader */
	for (int i = 0; i < _num_cpus && i < 64; i++) {
		if (_cpu_active[i]) count++;
	}
	return count;
}

/* Signal that this participant has finished scheduling */
void signal_scheduling_done(void) {
	pthread_mutex_lock(&_barrier_lock);
	_scheduling_done_count++;
	int total = count_active_participants();
	if (_scheduling_done_count >= total) {
		/* All participants done, release barrier */
		_scheduling_barrier_released = 1;
		pthread_cond_broadcast(&_barrier_cond);
	}
	pthread_mutex_unlock(&_barrier_lock);
}

/* Wait for all participants to finish scheduling */
void wait_scheduling_barrier(void) {
	pthread_mutex_lock(&_barrier_lock);
	while (!_scheduling_barrier_released) {
		pthread_cond_wait(&_barrier_cond, &_barrier_lock);
	}
	pthread_mutex_unlock(&_barrier_lock);
}




