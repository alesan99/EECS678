/** @file libscheduler.c
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 #include "libscheduler.h"
 #include "../libpriqueue/libpriqueue.h"
 
 // global variables to store scheduler state
 priqueue_t job_queue;
 scheme_t scheduling_scheme;
 int num_cores;
 int quantum;
 
 // array to track which job is running on each core
 typedef struct _core_t {
	 int job_id;         // job currently running on this core (-1 if idle)
	 int job_start_time; // time when the current job started running on this core
	 int time_remaining; // remaining execution time for the current job
 } core_t;
 
 core_t* cores;
 
 // statistics counters for calculating averages
 int total_jobs_completed = 0;
 float total_waiting_time = 0.0;
 float total_turnaround_time = 0.0;
 float total_response_time = 0.0;
 
 /**
   Stores information making up a job to be scheduled including any statistics.
 
   You may need to define some global variables or a struct to store your job queue elements. 
 */
 typedef struct _job_t
 {
	 int job_id;         // unique job identifier
	 int arrival_time;   // time when job arrived
	 int start_time;     // time when job first started executing (-1 if not started)
	 int remaining_time; // remaining execution time
	 int running_time;   // total execution time required
	 int priority;       // job priority (lower value = higher priority)
	 int core_id;        // core where job is running (-1 if not running)
	 int completion_time; // time when job completed
 } job_t;
 
 // comparison functions for different scheduling schemes
 int fcfs_compare(const void* a, const void* b) {
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 return job_a->arrival_time - job_b->arrival_time;
 }
 
 int sjf_compare(const void* a, const void* b) {
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 
	 if (job_a->running_time == job_b->running_time)
		 return job_a->arrival_time - job_b->arrival_time;
	 return job_a->running_time - job_b->running_time;
 }
 
 int psjf_compare(const void* a, const void* b) {
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 
	 if (job_a->remaining_time == job_b->remaining_time)
		 return job_a->arrival_time - job_b->arrival_time;
	 return job_a->remaining_time - job_b->remaining_time;
 }
 
 int pri_compare(const void* a, const void* b) {
	 job_t* job_a = (job_t*)a;
	 job_t* job_b = (job_t*)b;
	 
	 if (job_a->priority == job_b->priority)
		 return job_a->arrival_time - job_b->arrival_time;
	 return job_a->priority - job_b->priority;
 }
 
 int ppri_compare(const void* a, const void* b) {
	 // same as pri_compare since priority doesn't change
	 return pri_compare(a, b);
 }
 
 int rr_compare(const void* a, const void* b) {
	 // for round robin, we maintain the queue order
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
	 // set global variables
	 scheduling_scheme = scheme;
	 num_cores = cores;
	 quantum = 1; // default quantum for round robin
	 
	 // allocate and initialize core array
	 cores = (core_t*)malloc(sizeof(core_t) * num_cores);
	 for (int i = 0; i < num_cores; i++) {
		 cores[i].job_id = -1;  // -1 indicates idle core
		 cores[i].job_start_time = -1;
		 cores[i].time_remaining = 0;
	 }
	 
	 // initialize the priority queue with the appropriate comparator
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
			 priqueue_init(&job_queue, ppri_compare);
			 break;
		 case RR:
			 priqueue_init(&job_queue, rr_compare);
			 break;
		 default:
			 // should not reach here given assumptions
			 break;
	 }
 }
 
 // helper function to find an idle core
 // returns the core id or -1 if no idle cores are available
 int find_idle_core() {
	 for (int i = 0; i < num_cores; i++) {
		 if (cores[i].job_id == -1) {
			 return i;
		 }
	 }
	 return -1;
 }
 
 // helper function to check if a job should preempt the current running job on a core
 // returns 1 if the new job should preempt, 0 otherwise
 int should_preempt(job_t* new_job, int core_id) {
	 if (scheduling_scheme == PSJF) {
		 // for PSJF, preempt if new job has shorter remaining time
		 return new_job->remaining_time < cores[core_id].time_remaining;
	 } else if (scheduling_scheme == PPRI) {
		 // for PPRI, get the currently running job and compare priorities
		 job_t* current_job = NULL;
		 int queue_size = priqueue_size(&job_queue);
		 
		 // find the job with the matching job_id
		 for (int i = 0; i < queue_size; i++) {
			 job_t* job = (job_t*)priqueue_at(&job_queue, i);
			 if (job->job_id == cores[core_id].job_id) {
				 current_job = job;
				 break;
			 }
		 }
		 
		 if (current_job) {
			 // preempt if new job has higher priority (lower priority value)
			 return new_job->priority < current_job->priority;
		 }
	 }
	 
	 return 0; // don't preempt for non-preemptive schemes
 }
 
 // helper function to find the core with the highest priority job that can be preempted
 // returns the core id or -1 if no preemptable core is found
 int find_preemptable_core(job_t* new_job) {
	 int preempt_core = -1;
	 int highest_priority = -1;
	 int highest_remaining_time = -1;
	 
	 for (int i = 0; i < num_cores; i++) {
		 if (cores[i].job_id != -1 && should_preempt(new_job, i)) {
			 job_t* current_job = NULL;
			 int queue_size = priqueue_size(&job_queue);
			 
			 // find the job with the matching job_id
			 for (int j = 0; j < queue_size; j++) {
				 job_t* job = (job_t*)priqueue_at(&job_queue, j);
				 if (job->job_id == cores[i].job_id) {
					 current_job = job;
					 break;
				 }
			 }
			 
			 if (current_job) {
				 if (preempt_core == -1 || 
					 (scheduling_scheme == PPRI && current_job->priority > highest_priority) ||
					 (scheduling_scheme == PSJF && cores[i].time_remaining > highest_remaining_time)) {
					 preempt_core = i;
					 highest_priority = current_job->priority;
					 highest_remaining_time = cores[i].time_remaining;
				 }
			 }
		 }
	 }
	 
	 return preempt_core;
 }
 
 // helper function to assign a job to a core
 void assign_job_to_core(job_t* job, int core_id, int time) {
	 cores[core_id].job_id = job->job_id;
	 cores[core_id].time_remaining = job->remaining_time;
	 
	 // update job statistics
	 job->core_id = core_id;
	 
	 // set start time if this is the first time the job is running
	 if (job->start_time == -1) {
		 job->start_time = time;
	 }
 }
 
 // helper function to update a job's remaining time
 void update_job_remaining_time(job_t* job, int time_passed) {
	 job->remaining_time -= time_passed;
	 if (job->core_id != -1) {
		 cores[job->core_id].time_remaining = job->remaining_time;
	 }
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
	 job_t* new_job = (job_t*)malloc(sizeof(job_t));
	 new_job->job_id = job_number;
	 new_job->arrival_time = time;
	 new_job->start_time = -1; // not started yet
	 new_job->remaining_time = running_time;
	 new_job->running_time = running_time;
	 new_job->priority = priority;
	 new_job->core_id = -1; // not assigned to a core yet
	 new_job->completion_time = -1; // not completed yet
	 
	 // add job to queue
	 priqueue_offer(&job_queue, new_job);
	 
	 // check if there's an idle core
	 int idle_core = find_idle_core();
	 if (idle_core != -1) {
		 // assign the job to the idle core
		 assign_job_to_core(new_job, idle_core, time);
		 return idle_core;
	 }
	 
	 // no idle cores, check if we should preempt a running job
	 if (scheduling_scheme == PSJF || scheduling_scheme == PPRI) {
		 int preempt_core = find_preemptable_core(new_job);
		 if (preempt_core != -1) {
			 // update the current job on this core
			 int current_job_id = cores[preempt_core].job_id;
			 int queue_size = priqueue_size(&job_queue);
			 
			 // find and update the job being preempted
			 for (int i = 0; i < queue_size; i++) {
				 job_t* job = (job_t*)priqueue_at(&job_queue, i);
				 if (job->job_id == current_job_id) {
					 job->core_id = -1; // job is being preempted
					 // we don't update remaining time here - it will be done in the simulator
					 break;
				 }
			 }
			 
			 // assign the new job to the core
			 assign_job_to_core(new_job, preempt_core, time);
			 return preempt_core;
		 }
	 }
	 
	 // no preemption or idle cores
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
	 // find the completed job and remove it from the queue
	 int queue_size = priqueue_size(&job_queue);
	 job_t* completed_job = NULL;
	 
	 for (int i = 0; i < queue_size; i++) {
		 job_t* job = (job_t*)priqueue_at(&job_queue, i);
		 if (job->job_id == job_number) {
			 completed_job = job;
			 priqueue_remove(&job_queue, job);
			 break;
		 }
	 }
	 
	 if (completed_job) {
		 // update statistics
		 completed_job->completion_time = time;
		 total_jobs_completed++;
		 
		 // calculate waiting time (completion time - arrival time - running time)
		 float waiting_time = (float)(completed_job->completion_time - completed_job->arrival_time - completed_job->running_time);
		 total_waiting_time += waiting_time;
		 
		 // calculate turnaround time (completion time - arrival time)
		 float turnaround_time = (float)(completed_job->completion_time - completed_job->arrival_time);
		 total_turnaround_time += turnaround_time;
		 
		 // calculate response time (start time - arrival time)
		 float response_time = (float)(completed_job->start_time - completed_job->arrival_time);
		 total_response_time += response_time;
		 
		 // free the job memory
		 free(completed_job);
	 }
	 
	 // mark the core as idle
	 cores[core_id].job_id = -1;
	 cores[core_id].job_start_time = -1;
	 cores[core_id].time_remaining = 0;
	 
	 // check if there are any jobs in the queue that can be scheduled
	 if (priqueue_size(&job_queue) > 0) {
		 // get the highest priority job
		 job_t* next_job = (job_t*)priqueue_peek(&job_queue);
		 
		 // check if the job is already running on another core
		 if (next_job->core_id == -1) {
			 // job is not running, assign it to this core
			 assign_job_to_core(next_job, core_id, time);
			 return next_job->job_id;
		 } else {
			 // job is already running, look for the next available job
			 for (int i = 1; i < priqueue_size(&job_queue); i++) {
				 job_t* job = (job_t*)priqueue_at(&job_queue, i);
				 if (job->core_id == -1) {
					 assign_job_to_core(job, core_id, time);
					 return job->job_id;
				 }
			 }
		 }
	 }
	 
	 // no jobs to schedule
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
	 if (scheduling_scheme != RR) {
		 return cores[core_id].job_id; // return current job if not RR
	 }
	 
	 // find the current job on this core
	 int current_job_id = cores[core_id].job_id;
	 job_t* current_job = NULL;
	 
	 // remove the job from the queue
	 for (int i = 0; i < priqueue_size(&job_queue); i++) {
		 job_t* job = (job_t*)priqueue_at(&job_queue, i);
		 if (job->job_id == current_job_id) {
			 current_job = job;
			 priqueue_remove(&job_queue, job);
			 break;
		 }
	 }
	 
	 if (current_job) {
		 // update core status
		 cores[core_id].job_id = -1;
		 cores[core_id].job_start_time = -1;
		 
		 // put the job back at the end of the queue
		 current_job->core_id = -1;
		 priqueue_offer(&job_queue, current_job);
	 }
	 
	 // get the next job to run
	 if (priqueue_size(&job_queue) > 0) {
		 job_t* next_job = (job_t*)priqueue_peek(&job_queue);
		 
		 // if the next job is already running on another core, find the next available job
		 while (next_job->core_id != -1 && priqueue_size(&job_queue) > 1) {
			 // move this job to the end of the queue
			 priqueue_remove(&job_queue, next_job);
			 priqueue_offer(&job_queue, next_job);
			 
			 // get the next job
			 next_job = (job_t*)priqueue_peek(&job_queue);
		 }
		 
		 // if we found a job that's not already running, assign it to this core
		 if (next_job->core_id == -1) {
			 assign_job_to_core(next_job, core_id, time);
			 return next_job->job_id;
		 }
	 }
	 
	 // no jobs to schedule
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
	 if (total_jobs_completed == 0) {
		 return 0.0;
	 }
	 return total_waiting_time / total_jobs_completed;
 }
 
 /**
   Returns the average turnaround time of all jobs scheduled by your scheduler.
 
   Assumptions:
	 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
   @return the average turnaround time of all jobs scheduled.
  */
 float scheduler_average_turnaround_time()
 {
	 if (total_jobs_completed == 0) {
		 return 0.0;
	 }
	 return total_turnaround_time / total_jobs_completed;
 }
 
 /**
   Returns the average response time of all jobs scheduled by your scheduler.
 
   Assumptions:
	 - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
   @return the average response time of all jobs scheduled.
  */
 float scheduler_average_response_time()
 {
	 if (total_jobs_completed == 0) {
		 return 0.0;
	 }
	 return total_response_time / total_jobs_completed;
 }
 
 /**
   Free any memory associated with your scheduler.
  
   Assumption:
	 - This function will be the last function called in your library.
 */
 void scheduler_clean_up()
 {
	 // free remaining jobs in the queue (if any)
	 while (priqueue_size(&job_queue) > 0) {
		 job_t* job = (job_t*)priqueue_poll(&job_queue);
		 free(job);
	 }
	 
	 // destroy the priority queue
	 priqueue_destroy(&job_queue);
	 
	 // free the cores array
	 free(cores);
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
	 // print out jobs in the queue with their current core assignment
	 for (int i = 0; i < priqueue_size(&job_queue); i++) {
		 job_t* job = (job_t*)priqueue_at(&job_queue, i);
		 printf("%d(%d) ", job->job_id, job->core_id);
	 }
	 printf("\n");
 }