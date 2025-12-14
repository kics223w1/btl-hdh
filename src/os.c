
#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

static int time_slot;
static int num_cpus;
static int done = 0;
static struct krnl_t os;

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args {
	/* A dispatched argument struct to compact many-fields passing to loader */
	int vmemsz;
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	int active_mswp_id;
	struct timer_id_t  *timer_id;
};
#endif

static struct ld_args{
	char ** path;
	unsigned long * start_time;
#ifdef MLQ_SCHED
	unsigned long * prio;
#endif
} ld_processes;
int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};

/* Simple SIGSEGV handler to help debug rare crashes during tests */
static void sigsegv_handler(int sig)
{
	void *array[32];
	size_t size = backtrace(array, 32);

	fprintf(stderr, "\n*** Caught signal %d (segmentation fault) ***\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	_exit(1);
}


static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;
	while (1) {
		/* Wait for this CPU's turn - ensures deterministic ordering */
		/* CPUs process from highest ID to lowest within each time slot */
		wait_cpu_turn(id);
		
		/* Check the status of current process */
		if (proc == NULL) {
			/* No process is running, the we load new process from
		 	* ready queue */
			proc = get_proc();
			if (proc == NULL) {
				/* Signal next CPU before waiting for next slot */
				signal_next_cpu(id);
				next_slot(timer_id);
				continue; /* First load failed. skip dummy load */
			}
		}else if (proc->pc == proc->code->size) {
			/* The porcess has finish it job */
			printf("\tCPU %d: Processed %2d has finished\n",
				id ,proc->pid);
			free(proc);
			proc = get_proc();
			time_left = 0;
		}else if (time_left == 0) {
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n",
				id, proc->pid);
			put_proc(proc);
			proc = get_proc();
		}
		
		/* Recheck process status after loading new process */
		if (proc == NULL && done) {
			/* No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			mark_cpu_inactive(id);
			signal_next_cpu(id);
			break;
		}else if (proc == NULL) {
			/* There may be new processes to run in
			 * next time slots, just skip current slot */
			signal_next_cpu(id);
			next_slot(timer_id);
			continue;
		}else if (time_left == 0) {
			printf("\tCPU %d: Dispatched process %2d\n",
				id, proc->pid);
			time_left = time_slot;
		}
		
		/* Run current process */
		run(proc);
		
		/* Signal next CPU after completing scheduling work and running process */
		signal_next_cpu(id);
		
		time_left--;
		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
#ifdef MM_PAGING
	struct memphy_struct* mram = ((struct mmpaging_ld_args *)args)->mram;
	struct memphy_struct** mswp = ((struct mmpaging_ld_args *)args)->mswp;
	struct memphy_struct* active_mswp = ((struct mmpaging_ld_args *)args)->active_mswp;
	struct timer_id_t * timer_id = ((struct mmpaging_ld_args *)args)->timer_id;
#else
	struct timer_id_t * timer_id = (struct timer_id_t*)args;
#endif
	int i = 0;
	
	/* Loader runs after all CPUs (turn = -1) */
	wait_cpu_turn(-1);
	printf("ld_routine\n");
	signal_next_cpu(-1);
	
	while (i < num_processes) {
		struct pcb_t * proc = load(ld_processes.path[i]);
		struct krnl_t * krnl = proc->krnl = &os;	

#ifdef MLQ_SCHED
		proc->prio = ld_processes.prio[i];
#endif
		while (current_time() < ld_processes.start_time[i]) {
			next_slot(timer_id);
			/* Wait for loader's turn after all CPUs */
			wait_cpu_turn(-1);
			signal_next_cpu(-1);
		}
#ifdef MM_PAGING
		/* Initialize a fresh mm_struct before publishing it via krnl->mm
		 * to avoid other threads seeing a half‑initialised structure. */
		struct mm_struct *new_mm = malloc(sizeof(struct mm_struct));
		if (new_mm == NULL) {
			fprintf(stderr, "Failed to allocate mm_struct\n");
			exit(1);
		}
		init_mm(new_mm, proc);
		proc->mm = new_mm;
		krnl->mram = mram;
		krnl->mswp = mswp;
		krnl->active_mswp = active_mswp;
#endif
		printf("\tLoaded a process at %s, PID: %d PRIO: %ld\n",
			ld_processes.path[i], proc->pid, ld_processes.prio[i]);
		add_proc(proc);
		free(ld_processes.path[i]);
		i++;
		signal_next_cpu(-1);
		next_slot(timer_id);
		/* Wait for next loader turn */
		wait_cpu_turn(-1);
	}
	/* Signal completion before exiting */
	signal_next_cpu(-1);
	free(ld_processes.path);
	free(ld_processes.start_time);
	done = 1;
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void read_config(const char * path) {
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);
	ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
	ld_processes.start_time = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);
#ifdef MM_PAGING
	int sit;
	/* Init memory sizes with default values (legacy format) */
	memramsz = 0x100000;
	memswpsz[0] = 0x1000000;
	for (sit = 1; sit < PAGING_MAX_MMSWP; sit++)
		memswpsz[sit] = 0;

	/* Check if the next line contains memory configuration (new format) */
	long current_pos = ftell(file);
	char line[256];
	if (fgets(line, sizeof(line), file) != NULL) {
		int ram, swp[PAGING_MAX_MMSWP];
		int count = sscanf(line, "%d %d %d %d %d", &ram, &swp[0], &swp[1], &swp[2], &swp[3]);
		
		if (count == 5) {
			/* Found memory configuration line, update values */
			memramsz = ram;
#ifdef MM64
			/* Force minimum RAM size to 1 page if configured size is too small */
			if (memramsz < PAGING_PAGESZ) memramsz = PAGING_PAGESZ;
#endif
			for (sit = 0; sit < PAGING_MAX_MMSWP; sit++)
				memswpsz[sit] = swp[sit];
		} else {
			/* Not a memory config line (likely process description), rewind */
			fseek(file, current_pos, SEEK_SET);
		}
	} else {
		/* End of file or error, but we haven't read processes yet? 
		   Just rewind to be safe, though likely an invalid file if empty here. */
		fseek(file, current_pos, SEEK_SET);
	}
#endif

#ifdef MLQ_SCHED
	ld_processes.prio = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);
#endif
	int i;
	for (i = 0; i < num_processes; i++) {
		ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
		ld_processes.path[i][0] = '\0';
		strcat(ld_processes.path[i], "input/proc/");
		char proc[100];
#ifdef MLQ_SCHED
		fscanf(file, "%lu %s %lu\n", &ld_processes.start_time[i], proc, &ld_processes.prio[i]);
#else
		fscanf(file, "%lu %s\n", &ld_processes.start_time[i], proc);
#endif
		strcat(ld_processes.path[i], proc);
	}
}

int main(int argc, char * argv[]) {
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}

	/* Install debug SIGSEGV handler (has no effect when program exits
	 * normally, but gives us a backtrace if a rare crash occurs). */
	signal(SIGSEGV, sigsegv_handler);

	char path[100];
	path[0] = '\0';
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args * args =
		(struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;
	
	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++) {
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t * ld_event = attach_event();
	
	/* Initialize CPU ordering for deterministic scheduling */
	init_cpu_order(num_cpus);
	
	start_timer();

#ifdef MM_PAGING
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */
	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */

	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];

	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);

        /* Create all MEM SWAP */ 
	int sit;
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
	       init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));

	mm_ld_args->timer_id = ld_event;
	mm_ld_args->mram = (struct memphy_struct *) &mram;
	mm_ld_args->mswp = (struct memphy_struct**) &mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *) &mswp[0];
        mm_ld_args->active_mswp_id = 0;
#endif

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader */
#ifdef MM_PAGING
	pthread_create(&ld, NULL, ld_routine, (void*)mm_ld_args);
#else
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
#endif
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cpu_routine, (void*)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();

	return 0;

}



