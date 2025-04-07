/** @file libscheduler.c
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 #include "libscheduler.h"
 #include "../libpriqueue/libpriqueue.h"
 
 // global variables to track scheduler state
 scheme_t scheduler_scheme;
 int scheduler_cores;
 priqueue_t job_queue;
 
 // array to track which job is running on each core
 int* core_job_map;
 
 // track statistics for calculating averages
 int total_jobs = 0;
 float total_waiting_time = 0.0;
 float total_turnaround_time = 0.0;
 float total_response_time = 0.0;
 
 /**
   Stores information making up a job to be scheduled including any statistics.
 
   You may need to define some global variables or a struct to store your job queue elements. 
 */
 typedef struct _job_t
 {
	 int job_id;             // unique identifier for the job
	 int arrival_time;       // time when the job arrived
	 int run_time;           // total execution time needed
	 int priority;           // job priority (lower value = higher priority)
	 int remaining_time;     // time left to complete the job
	 int first_run_time;     // time when job first started running (-1 if not run yet)
	 int completion_time;    // time when job completed (-1 if not completed)
	 int current_core;       // core where job is currently running (-1 if not running)
 } job_t;
 
 // comparison functions for different scheduling algorithms
 int fcfs_compare(const void* a, const void* b) {
	 // order by arrival time (FCFS)
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 return job_a->arrival_time - job_b->arrival_time;
 }
 
 int sjf_compare(const void* a, const void* b) {
	 // order by total run time, with ties broken by arrival time
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 
	 if (job_a->run_time == job_b->run_time)
		 return job_a->arrival_time - job_b->arrival_time;
	 return job_a->run_time - job_b->run_time;
 }
 
 int psjf_compare(const void* a, const void* b) {
	 // order by remaining time, with ties broken by arrival time
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 
	 if (job_a->remaining_time == job_b->remaining_time)
		 return job_a->arrival_time - job_b->arrival_time;
	 return job_a->remaining_time - job_b->remaining_time;
 }
 
 int pri_compare(const void* a, const void* b) {
	 // order by priority, with ties broken by arrival time
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 
	 if (job_a->priority == job_b->priority)
		 return job_a->arrival_time - job_b->arrival_time;
	 return job_a->priority - job_b->priority;
 }
 
 int rr_compare(const void* a, const void* b) {
	 // for round robin, we just use arrival time
	 // this will be manipulated to create the round-robin behavior
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 return job_a->arrival_time - job_b->arrival_time;
 }
 
 /**
   Initalizes the scheduler.
  
   Assumptions:
	 - You may assume this will be the first scheduler function called.
	 - You may assume this function will be called once once.
	 - You may assume that cores is a positive, non-zero number.
	 - You may assume that scheme is a valid scheduling scheme.
 
   @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
   @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
 */
 void scheduler_start_up(int cores, scheme_t scheme)
 {
	 // store the scheduler parameters
	 scheduler_cores = cores;
	 scheduler_scheme = scheme;
	 
	 // initialize the tracking array for cores
	 core_job_map = malloc(sizeof(int) * cores);
	 for (int i = 0; i < cores; i++) {
		 core_job_map[i] = -1;  // -1 means no job assigned
	 }
	 
	 // initialize the priority queue with the appropriate comparison function
	 switch (scheme) {
		 case FCFS:
			 priqueue_init(&job_queue, fcfs_compare);
			 break;
		 case SJF:
			 priqueue_init(&job_queue, sjf_compare);
			 break;
		 case PSJF:
			 priqueue_init(&job_queue, psjf_compare);
			 break;
		 case PRI:
			 priqueue_init(&job_queue, pri_compare);
			 break;
		 case PPRI:
			 priqueue_init(&job_queue, pri_compare);  // same comparator as PRI
			 break;
		 case RR:
			 priqueue_init(&job_queue, rr_compare);
			 break;
	 }
 }
 
 // helper function to find an available core
 // returns the core id or -1 if no cores are available
 int find_available_core() {
	 for (int i = 0; i < scheduler_cores; i++) {
		 if (core_job_map[i] == -1) {
			 return i;
		 }
	 }
	 return -1;
 }
 
 // helper function to find the job with a specific id
 job_t* find_job_by_id(int job_id) {
	 int size = priqueue_size(&job_queue);
	 for (int i = 0; i < size; i++) {
		 job_t* job = (job_t*)priqueue_at(&job_queue, i);
		 if (job->job_id == job_id) {
			 return job;
		 }
	 }
	 return NULL;
 }
 
 // helper function to determine if a job should preempt another job
 int should_preempt(job_t* new_job, job_t* current_job) {
	 if (scheduler_scheme == PSJF) {
		 return new_job->remaining_time < current_job->remaining_time;
	 }
	 else if (scheduler_scheme == PPRI) {
		 return new_job->priority < current_job->priority;
	 }
	 return 0;  // no preemption for non-preemptive schedulers
 }
 
 /**
   Called when a new job arrives.
  
   If multiple cores are idle, the job should be assigned to the core with the
   lowest id.
   If the job arriving should be scheduled to run during the next
   time cycle, return the zero-based index of the core the job should be
   scheduled on. If another job is already running on the core specified,
   this will preempt the currently running job.
   Assumption:
	 - You may assume that every job wil have a unique arrival time.
 
   @param job_number a globally unique identification number of the job arriving.
   @param time the current time of the simulator.
   @param running_time the total number of time units this job will run before it will be finished.
   @param priority the priority of the job. (The lower the value, the higher the priority.)
   @return index of core job should be scheduled on
   @return -1 if no scheduling changes should be made. 
  
  */
 int scheduler_new_job(int job_number, int time, int running_time, int priority)
 {
	 // create a new job
	 job_t* new_job = malloc(sizeof(job_t));
	 new_job->job_id = job_number;
	 new_job->arrival_time = time;
	 new_job->run_time = running_time;
	 new_job->remaining_time = running_time;
	 new_job->priority = priority;
	 new_job->first_run_time = -1;
	 new_job->completion_time = -1;
	 new_job->current_core = -1;
	 
	 // add the job to our statistics counter
	 total_jobs++;
	 
	 // add the job to the queue
	 priqueue_offer(&job_queue, new_job);
	 
	 // first check if any core is available
	 int core = find_available_core();
	 if (core != -1) {
		 // assign the job to this core
		 core_job_map[core] = job_number;
		 new_job->current_core = core;
		 return core;
	 }
	 
	 // if no cores are available and this is a preemptive scheme,
	 // check if this job should preempt any running job
	 if (scheduler_scheme == PSJF || scheduler_scheme == PPRI) {
		 // find the job with the lowest priority (highest value) or longest remaining time
		 int preempt_core = -1;
		 job_t* preempt_job = NULL;
		 
		 for (int i = 0; i < scheduler_cores; i++) {
			 int running_job_id = core_job_map[i];
			 job_t* running_job = find_job_by_id(running_job_id);
			 
			 if (running_job && should_preempt(new_job, running_job)) {
				 if (preempt_core == -1 || should_preempt(running_job, preempt_job)) {
					 preempt_core = i;
					 preempt_job = running_job;
				 }
			 }
		 }
		 
		 if (preempt_core != -1) {
			 // update the preempted job
			 preempt_job->current_core = -1;
			 
			 // assign the new job to this core
			 core_job_map[preempt_core] = job_number;
			 new_job->current_core = preempt_core;
			 return preempt_core;
		 }
	 }
	 
	 // no preemption or available cores
	 return -1;
 }
 
 /**
   Called when a job has completed execution.
  
   The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
   If any job should be scheduled to run on the core free'd up by the
   finished job, return the job_number of the job that should be scheduled to
   run on core core_id.
  
   @param core_id the zero-based index of the core where the job was located.
   @param job_number a globally unique identification number of the job.
   @param time the current time of the simulator.
   @return job_number of the job that should be scheduled to run on core core_id
   @return -1 if core should remain idle.
  */
 int scheduler_job_finished(int core_id, int job_number, int time)
 {
	 // find the job that just finished
	 job_t* finished_job = find_job_by_id(job_number);
	 if (finished_job) {
		 // mark the job as completed
		 finished_job->completion_time = time;
		 
		 // update statistics
		 float turnaround_time = time - finished_job->arrival_time;
		 float waiting_time = turnaround_time - finished_job->run_time;
		 float response_time = finished_job->first_run_time - finished_job->arrival_time;
		 
		 total_turnaround_time += turnaround_time;
		 total_waiting_time += waiting_time;
		 total_response_time += response_time;
		 
		 // remove the job from the queue
		 priqueue_remove(&job_queue, finished_job);
		 free(finished_job);
	 }
	 
	 // mark the core as available
	 core_job_map[core_id] = -1;
	 
	 // see if there's a job that can run on this core
	 if (priqueue_size(&job_queue) > 0) {
		 // get the highest priority job that isn't running
		 int queue_size = priqueue_size(&job_queue);
		 for (int i = 0; i < queue_size; i++) {
			 job_t* next_job = (job_t*)priqueue_at(&job_queue, i);
			 if (next_job->current_core == -1) {
				 // found a job that isn't running
				 next_job->current_core = core_id;
				 core_job_map[core_id] = next_job->job_id;
				 
				 // if this is the first time the job runs, record the time
				 if (next_job->first_run_time == -1) {
					 next_job->first_run_time = time;
				 }
				 
				 return next_job->job_id;
			 }
		 }
	 }
	 
	 // no jobs available to run
	 return -1;
 }
 
 /**
   When the scheme is set to RR, called when the quantum timer has expired
   on a core.
  
   If any job should be scheduled to run on the core free'd up by
   the quantum expiration, return the job_number of the job that should be
   scheduled to run on core core_id.
 
   @param core_id the zero-based index of the core where the quantum has expired.
   @param time the current time of the simulator. 
   @return job_number of the job that should be scheduled on core cord_id
   @return -1 if core should remain idle
  */
 int scheduler_quantum_expired(int core_id, int time)
 {
	 // only relevant for Round Robin
	 if (scheduler_scheme != RR) {
		 return core_job_map[core_id]; // keep the current job
	 }
	 
	 // find the job currently running on this core
	 int current_job_id = core_job_map[core_id];
	 job_t* current_job = find_job_by_id(current_job_id);
	 
	 if (current_job) {
		 // move this job to the back of the queue by removing and re-adding it
		 priqueue_remove(&job_queue, current_job);
		 
		 // update the job's virtual arrival time to ensure it goes to the back
		 current_job->arrival_time = time;
		 
		 // mark the job as not running on any core
		 current_job->current_core = -1;
		 
		 // add it back to the queue
		 priqueue_offer(&job_queue, current_job);
	 }
	 
	 // mark the core as available
	 core_job_map[core_id] = -1;
	 
	 // find the next job to run (the job at the front of the queue that isn't running)
	 int queue_size = priqueue_size(&job_queue);
	 for (int i = 0; i < queue_size; i++) {
		 job_t* next_job = (job_t*)priqueue_at(&job_queue, i);
		 if (next_job->current_core == -1) {
			 // assign this job to the core
			 next_job->current_core = core_id;
			 core_job_map[core_id] = next_job->job_id;
			 
			 // if this is the first time the job runs, record the time
			 if (next_job->first_run_time == -1) {
				 next_job->first_run_time = time;
			 }
			 
			 return next_job->job_id;
		 }
	 }
	 
	 // no jobs available to run
	 return -1;
 }
 
 /**
   Returns the average waiting time of all jobs scheduled by your scheduler.
 
   Assumptions:
	 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
   @return the average waiting time of all jobs scheduled.
  */
 float scheduler_average_waiting_time()
 {
	 if (total_jobs == 0) return 0.0;
	 return total_waiting_time / total_jobs;
 }
 
 /**
   Returns the average turnaround time of all jobs scheduled by your scheduler.
 
   Assumptions:
	 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
   @return the average turnaround time of all jobs scheduled.
  */
 float scheduler_average_turnaround_time()
 {
	 if (total_jobs == 0) return 0.0;
	 return total_turnaround_time / total_jobs;
 }
 
 /**
   Returns the average response time of all jobs scheduled by your scheduler.
 
   Assumptions:
	 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
   @return the average response time of all jobs scheduled.
  */
 float scheduler_average_response_time()
 {
	 if (total_jobs == 0) return 0.0;
	 return total_response_time / total_jobs;
 }
 
 /**
   Free any memory associated with your scheduler.
  
   Assumption:
	 - This function will be the last function called in your library.
 */
 void scheduler_clean_up()
 {
	 // free all remaining jobs in the queue
	 while (priqueue_size(&job_queue) > 0) {
		 job_t* job = priqueue_poll(&job_queue);
		 free(job);
	 }
	 
	 // destroy the priority queue
	 priqueue_destroy(&job_queue);
	 
	 // free the core job map
	 free(core_job_map);
 }
 
 /**
   This function may print out any debugging information you choose. This
   function will be called by the simulator after every call the simulator
   makes to your scheduler.
   In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:
 
	 2(-1) 4(0) 1(-1)  
   
   This function is not required and will not be graded. You may leave it
   blank if you do not find it useful.
  */
 void scheduler_show_queue()
 {
	 // print out the queue in order
	 int size = priqueue_size(&job_queue);
	 for (int i = 0; i < size; i++) {
		 job_t* job = (job_t*)priqueue_at(&job_queue, i);
		 printf("%d(%d) ", job->job_id, job->current_core);
	 }
	 printf("\n");
 }