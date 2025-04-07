/** @file libscheduler.c
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stdbool.h>
 
 #include "libscheduler.h"
 #include "../libpriqueue/libpriqueue.h"
 
 // Global variables
 scheme_t scheduling_scheme;
 int num_cores;
 int total_waiting_time = 0;
 int total_turnaround_time = 0;
 int total_response_time = 0;
 int total_jobs = 0;
 
 /**
   Stores information making up a job to be scheduled including any statistics.
 
   You may need to define some global variables or a struct to store your job queue elements. 
 */
 typedef struct _job_t
 {
     int job_number;          // unique id for the job
     int arrival_time;        // when the job arrived
     int running_time;        // total execution time needed
     int remaining_time;      // time left to complete the job
     int priority;            // priority value (lower is higher priority)
     int start_time;          // when job first started running (-1 if not started)
     int finish_time;         // when job completes (-1 if not completed)
     int last_run_time;       // when the job last ran on CPU
     int core_id;             // which core the job is running on (-1 if not running)
 } job_t;
 
 // Struct to represent CPU cores
 typedef struct _core_t {
     job_t *current_job;     // pointer to job currently running on this core (NULL if idle)
     int time_slice;         // for Round Robin, tracks how long current job has run
 } core_t;
 
 // Global variables
 priqueue_t job_queue;     // queue of waiting jobs
 core_t *cores;            // array of cores
 
 // Comparison functions for different scheduling schemes
 int fcfs_compare(const void *a, const void *b) {
     job_t *job_a = (job_t*)a;
     job_t *job_b = (job_t*)b;
     return job_a->arrival_time - job_b->arrival_time;
 }
 
 int sjf_compare(const void *a, const void *b) {
     job_t *job_a = (job_t*)a;
     job_t *job_b = (job_t*)b;
     if (job_a->running_time == job_b->running_time)
         return job_a->arrival_time - job_b->arrival_time;
     return job_a->running_time - job_b->running_time;
 }
 
 int psjf_compare(const void *a, const void *b) {
     job_t *job_a = (job_t*)a;
     job_t *job_b = (job_t*)b;
     if (job_a->remaining_time == job_b->remaining_time)
         return job_a->arrival_time - job_b->arrival_time;
     return job_a->remaining_time - job_b->remaining_time;
 }
 
 int pri_compare(const void *a, const void *b) {
     job_t *job_a = (job_t*)a;
     job_t *job_b = (job_t*)b;
     if (job_a->priority == job_b->priority)
         return job_a->arrival_time - job_b->arrival_time;
     return job_a->priority - job_b->priority;
 }
 
 int ppri_compare(const void *a, const void *b) {
     // Same as pri_compare since both prioritize by priority value
     return pri_compare(a, b);
 }
 
 int rr_compare(const void *a, const void *b) {
     // Round Robin uses FIFO ordering
     return fcfs_compare(a, b);
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
     // Initialize globals
     scheduling_scheme = scheme;
     
     // allocate memory for cores
     this_cores = malloc(sizeof(core_t) * num_cores);
     
     // Initialize all cores as idle
     for (int i = 0; i < num_cores; i++) {
         this_cores[i].current_job = NULL;
         this_cores[i].time_slice = 0;
     }
     
     // choose appropriate comparison function based on scheduling scheme
     int (*comparer)(const void *, const void *);
     
     switch(scheme) {
         case FCFS:
             comparer = fcfs_compare;
             break;
         case SJF:
             comparer = sjf_compare;
             break;
         case PSJF:
             comparer = psjf_compare;
             break;
         case PRI:
             comparer = pri_compare;
             break;
         case PPRI:
             comparer = ppri_compare;
             break;
         case RR:
             comparer = rr_compare;
             break;
         default:
             // shouldnt happen due to assumptions, but fallback to FCFS
             comparer = fcfs_compare;
     }
     
     // Initialize priority queue with appropriate comparison function
     priqueue_init(&job_queue, comparer);
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
     // Create new job
     job_t *new_job = (job_t*)malloc(sizeof(job_t));
     new_job->job_number = job_number;
     new_job->arrival_time = time;
     new_job->running_time = running_time;
     new_job->remaining_time = running_time;
     new_job->priority = priority;
     new_job->start_time = -1;  // Not started yet
     new_job->finish_time = -1; // Not finished yet
     new_job->last_run_time = -1; // Never run
     new_job->core_id = -1;     // Not assigned to any core
     
     // Track total jobs for stats
     total_jobs++;
     
     // First, check if there's an idle core
     int idle_core = -1;
     for (int i = 0; i < num_cores; i++) {
         if (cores[i].current_job == NULL) {
             idle_core = i;
             break; // Use the lowest-numbered idle core
         }
     }
     
     // If we found an idle core, schedule the job there
     if (idle_core != -1) {
         cores[idle_core].current_job = new_job;
         new_job->core_id = idle_core;
         
         // Reset time slice for RR
         if (scheduling_scheme == RR) {
             cores[idle_core].time_slice = 0;
         }
         
         return idle_core;
     }
     
     // Handle preemptive scheduling schemes
     if (scheduling_scheme == PSJF || scheduling_scheme == PPRI) {
         int target_core = -1;
         job_t *worst_job = NULL;
         
         // Find worst job currently running on any core based on scheme
         for (int i = 0; i < num_cores; i++) {
             job_t *current = cores[i].current_job;
             
             // Initialize worst_job if not set
             if (worst_job == NULL) {
                 worst_job = current;
                 target_core = i;
                 continue;
             }
             
             // Compare based on scheme
             int comparison;
             if (scheduling_scheme == PSJF) {
                 // For PSJF, compare remaining times
                 if (current->remaining_time > worst_job->remaining_time ||
                     (current->remaining_time == worst_job->remaining_time && 
                      current->arrival_time > worst_job->arrival_time)) {
                     worst_job = current;
                     target_core = i;
                 }
             }
             else if (scheduling_scheme == PPRI) {
                 // For PPRI, compare priorities
                 if (current->priority > worst_job->priority ||
                     (current->priority == worst_job->priority && 
                      current->arrival_time > worst_job->arrival_time)) {
                     worst_job = current;
                     target_core = i;
                 }
             }
         }
         
         // Now compare new job with the worst job
         bool should_preempt = false;
         
         if (scheduling_scheme == PSJF) {
             should_preempt = (new_job->remaining_time < worst_job->remaining_time);
         }
         else if (scheduling_scheme == PPRI) {
             should_preempt = (new_job->priority < worst_job->priority);
         }
         
         // Preempt if needed
         if (should_preempt) {
             // Put the preempted job back in the queue
             worst_job->core_id = -1;
             priqueue_offer(&job_queue, worst_job);
             
             // Assign new job to the core
             cores[target_core].current_job = new_job;
             new_job->core_id = target_core;
             
             // Reset time slice for RR (though this doesn't apply for PSJF/PPRI)
             cores[target_core].time_slice = 0;
             
             return target_core;
         }
     }
     
     // If not scheduled immediately, add to queue
     priqueue_offer(&job_queue, new_job);
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
     // Get the job that just finished
     job_t *finished_job = cores[core_id].current_job;
     
     // Update statistics
     finished_job->finish_time = time;
     
     // Calculate and accumulate statistics
     int turnaround_time = finished_job->finish_time - finished_job->arrival_time;
     int waiting_time = turnaround_time - finished_job->running_time;
     int response_time = finished_job->start_time - finished_job->arrival_time;
     
     total_turnaround_time += turnaround_time;
     total_waiting_time += waiting_time;
     total_response_time += response_time;
     
     // Free the core
     cores[core_id].current_job = NULL;
     
     // Free the job memory
     free(finished_job);
     
     // If there are jobs waiting, schedule the next one
     if (priqueue_size(&job_queue) > 0) {
         job_t *next_job = (job_t*)priqueue_poll(&job_queue);
         
         // If job hasn't started before, record start time
         if (next_job->start_time == -1) {
             next_job->start_time = time;
         }
         
         // Update job and core state
         next_job->core_id = core_id;
         next_job->last_run_time = time;
         cores[core_id].current_job = next_job;
         
         // Reset time slice for RR
         if (scheduling_scheme == RR) {
             cores[core_id].time_slice = 0;
         }
         
         return next_job->job_number;
     }
     
     // No jobs waiting
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
     // This function is only relevant for RR scheduling
     if (scheduling_scheme != RR) {
         return -1;
     }
     
     // Get the current job on the core
     job_t *current_job = cores[core_id].current_job;
     
     // If there's no job or no other jobs waiting, keep running current job
     if (current_job == NULL || priqueue_size(&job_queue) == 0) {
         return -1;
     }
     
     // Put current job at the end of the queue
     current_job->core_id = -1;
     priqueue_offer(&job_queue, current_job);
     
     // Get the next job from the queue
     job_t *next_job = (job_t*)priqueue_poll(&job_queue);
     
     // If job hasn't started before, record start time
     if (next_job->start_time == -1) {
         next_job->start_time = time;
     }
     
     // Update job and core state
     next_job->core_id = core_id;
     next_job->last_run_time = time;
     cores[core_id].current_job = next_job;
     cores[core_id].time_slice = 0;  // Reset time slice
     
     return next_job->job_number;
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
     return (float)total_waiting_time / total_jobs;
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
     return (float)total_turnaround_time / total_jobs;
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
     return (float)total_response_time / total_jobs;
 }
 
 
 /**
   Free any memory associated with your scheduler.
  
   Assumption:
     - This function will be the last function called in your library.
 */
 void scheduler_clean_up()
 {
     // free any remaining jobs in the queue
     while (priqueue_size(&job_queue) > 0) {
         job_t *job = (job_t*)priqueue_poll(&job_queue);
         free(job);
     }
     
     // free any jobs still running on cores
     for (int i = 0; i < num_cores; i++) {
         if (cores[i].current_job != NULL) {
             free(cores[i].current_job);
             cores[i].current_job = NULL;
         }
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
     // Print jobs in the queue - this is for debugging only
     printf("Queue: ");
     
     // Print jobs in the ready queue
     int queue_size = priqueue_size(&job_queue);
     for (int i = 0; i < queue_size; i++) {
         job_t *job = (job_t*)priqueue_at(&job_queue, i);
         printf("%d(-1) ", job->job_number);
     }
     
     // Print jobs running on cores
     for (int i = 0; i < num_cores; i++) {
         if (cores[i].current_job != NULL) {
             printf("%d(%d) ", cores[i].current_job->job_number, i);
         }
     }
     
     printf("\n");
 }