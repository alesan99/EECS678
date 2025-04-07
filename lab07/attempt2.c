/** @file libscheduler.c
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 #include "libscheduler.h"
 #include "../libpriqueue/libpriqueue.h"
 
 
 /**
   Stores information making up a job to be scheduled including any statistics.
 
   You may need to define some global variables or a struct to store your job queue elements. 
  */
 typedef struct _job_t
 {
   int job_number;
   int arrival_time;
   int running_time;
   int remaining_time;
   int priority;
   int start_time;
   int finish_time;
 } job_t;
 
 /* Global variables */
 static priqueue_t job_queue;
 static scheme_t scheduling_scheme;
 static int num_cores;
 static job_t **cores; // Array to track jobs running on each core
 static int total_waiting_time = 0;
 static int total_turnaround_time = 0;
 static int total_response_time = 0;
 static int total_jobs = 0;
 
 /* Comparison functions for different scheduling schemes */
 int compare_fcfs(const void *a, const void *b)
 {
   job_t *job_a = (job_t *)a;
   job_t *job_b = (job_t *)b;
   return job_a->arrival_time - job_b->arrival_time;
 }
 
 int compare_sjf(const void *a, const void *b)
 {
   job_t *job_a = (job_t *)a;
   job_t *job_b = (job_t *)b;
   return job_a->running_time - job_b->running_time;
 }
 
 int compare_psjf(const void *a, const void *b)
 {
   job_t *job_a = (job_t *)a;
   job_t *job_b = (job_t *)b;
   return job_a->remaining_time - job_b->remaining_time;
 }
 
 int compare_pri(const void *a, const void *b)
 {
   job_t *job_a = (job_t *)a;
   job_t *job_b = (job_t *)b;
   if (job_a->priority == job_b->priority)
     return job_a->arrival_time - job_b->arrival_time;
   return job_a->priority - job_b->priority;
 }
 
 int compare_rr(const void *a, const void *b)
 {
   return 0; // RR does not require sorting
 }
 
 /**
   Initializes the scheduler.
  */
 void scheduler_start_up(int cores_count, scheme_t scheme)
 {
   num_cores = cores_count;
   scheduling_scheme = scheme;
   cores = malloc(sizeof(job_t *) * num_cores);
   for (int i = 0; i < num_cores; i++)
     cores[i] = NULL;
 
   switch (scheme)
   {
   case FCFS:
     priqueue_init(&job_queue, compare_fcfs);
     break;
   case SJF:
     priqueue_init(&job_queue, compare_sjf);
     break;
   case PSJF:
     priqueue_init(&job_queue, compare_psjf);
     break;
   case PRI:
   case PPRI:
     priqueue_init(&job_queue, compare_pri);
     break;
   case RR:
     priqueue_init(&job_queue, compare_rr);
     break;
   }
 }
 
 /**
   Called when a new job arrives.
  */
 int scheduler_new_job(int job_number, int time, int running_time, int priority)
 {
   job_t *new_job = malloc(sizeof(job_t));
   new_job->job_number = job_number;
   new_job->arrival_time = time;
   new_job->running_time = running_time;
   new_job->remaining_time = running_time;
   new_job->priority = priority;
   new_job->start_time = -1;
   new_job->finish_time = -1;
 
   // Check for an available core
   for (int i = 0; i < num_cores; i++)
   {
     if (cores[i] == NULL)
     {
       cores[i] = new_job;
       new_job->start_time = time;
       total_response_time += time - new_job->arrival_time;
       return i;
     }
   }
 
   // Add the job to the queue
   priqueue_offer(&job_queue, new_job);
 
   // Preempt if necessary (for PSJF or PPRI)
   if (scheduling_scheme == PSJF || scheduling_scheme == PPRI)
   {
     int preempt_core = -1;
     for (int i = 0; i < num_cores; i++)
     {
       if (cores[i] != NULL && compare_psjf(new_job, cores[i]) < 0)
       {
         preempt_core = i;
         break;
       }
     }
 
     if (preempt_core != -1)
     {
       job_t *preempted_job = cores[preempt_core];
       cores[preempt_core] = new_job;
       new_job->start_time = time;
       total_response_time += time - new_job->arrival_time;
       priqueue_offer(&job_queue, preempted_job);
       return preempt_core;
     }
   }
 
   return -1;
 }
 
 /**
   Called when a job has completed execution.
  */
  int scheduler_job_finished(int core_id, int job_number, int time)
  {
    // Ensure the core has a valid job
    if (cores[core_id] == NULL)
    {
      fprintf(stderr, "Error: Core %d has no job to finish.\n", core_id);
      return -1;
    }

    job_t *finished_job = cores[core_id];

    // Validate the job being finished
    if (finished_job->job_number != job_number)
    {
      fprintf(stderr, "Error: Job number mismatch on core %d. Expected %d, got %d.\n",
              core_id, finished_job->job_number, job_number);
      return -1;
    }

    // Update statistics
    cores[core_id] = NULL;
    finished_job->finish_time = time;
    total_turnaround_time += time - finished_job->arrival_time;
    total_waiting_time += time - finished_job->arrival_time - finished_job->running_time;
    total_jobs++;

    // Free the finished job
    free(finished_job);

    // Schedule the next job
    job_t *next_job = priqueue_poll(&job_queue);
    if (next_job != NULL)
    {
      cores[core_id] = next_job;

      // Update response time if the job is running for the first time
      if (next_job->start_time == -1)
      {
        next_job->start_time = time;
        total_response_time += time - next_job->arrival_time;
      }

      return next_job->job_number;
    }

    return -1; // No job to schedule
  }
 
 /**
   When the scheme is set to RR, called when the quantum timer has expired.
  */
 int scheduler_quantum_expired(int core_id, int time)
 {
   job_t *expired_job = cores[core_id];
   cores[core_id] = NULL;
 
   priqueue_offer(&job_queue, expired_job);
 
   job_t *next_job = priqueue_poll(&job_queue);
   if (next_job != NULL)
   {
     cores[core_id] = next_job;
     if (next_job->start_time == -1)
       total_response_time += time - next_job->arrival_time;
     return next_job->job_number;
   }
 
   return -1;
 }
 
 /**
   Returns the average waiting time of all jobs scheduled by your scheduler.
  */
 float scheduler_average_waiting_time()
 {
   return (float)total_waiting_time / total_jobs;
 }
 
 /**
   Returns the average turnaround time of all jobs scheduled by your scheduler.
  */
 float scheduler_average_turnaround_time()
 {
   return (float)total_turnaround_time / total_jobs;
 }
 
 /**
   Returns the average response time of all jobs scheduled by your scheduler.
  */
 float scheduler_average_response_time()
 {
   return (float)total_response_time / total_jobs;
 }
 
 /**
   Free any memory associated with your scheduler.
  */
 void scheduler_clean_up()
 {
   priqueue_destroy(&job_queue);
   free(cores);
 }
 
 /**
   This function may print out any debugging information you choose.
  */
 void scheduler_show_queue()
 {
   // Optional debugging function
 }