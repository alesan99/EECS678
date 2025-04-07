/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

typedef struct _job_t
{
  int job_number;
  int arrival_time;
  int priority;
  int burst_time;
  int remaining_time;
  int start_time;
  int finish_time;
} job_t;

typedef struct _core_t
{
  int id;
  bool idle;
  job_t *current_job;
} core_t;

static priqueue_t job_queue;
static core_t *cores;
static int num_cores;
static scheme_t scheduling_scheme;

static int total_waiting_time = 0;
static int total_turnaround_time = 0;
static int total_response_time = 0;
static int total_jobs = 0;

int fcfs_compare(const void *l, const void *r)
{
  return ((job_t *)l)->arrival_time - ((job_t *)r)->arrival_time;
}

int sjf_compare(const void *l, const void *r)
{
  return ((job_t *)l)->burst_time - ((job_t *)r)->burst_time;
}

int priority_compare(const void *l, const void *r)
{
  return ((job_t *)l)->priority - ((job_t *)r)->priority;
}

int rr_compare(const void *l, const void *r)
{
  return 1; // Round Robin does not require sorting
}

void scheduler_start_up(int cores_count, scheme_t scheme)
{
  num_cores = cores_count;
  scheduling_scheme = scheme;

  cores = malloc(sizeof(core_t) * num_cores);
  for (int i = 0; i < num_cores; i++)
  {
    cores[i].id = i;
    cores[i].idle = true;
    cores[i].current_job = NULL;
  }

  switch (scheme)
  {
  case FCFS:
    priqueue_init(&job_queue, fcfs_compare);
    break;
  case SJF:
    priqueue_init(&job_queue, sjf_compare);
    break;
  case PSJF:
    priqueue_init(&job_queue, sjf_compare);
    break;
  case PRI:
    priqueue_init(&job_queue, priority_compare);
    break;
  case PPRI:
    priqueue_init(&job_queue, priority_compare);
    break;
  case RR:
    priqueue_init(&job_queue, rr_compare);
    break;
  }
}

int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  job_t *job = malloc(sizeof(job_t));
  job->job_number = job_number;
  job->arrival_time = time;
  job->priority = priority;
  job->burst_time = running_time;
  job->remaining_time = running_time;
  job->start_time = -1;
  job->finish_time = -1;

  for (int i = 0; i < num_cores; i++)
  {
    if (cores[i].idle)
    {
      cores[i].idle = false;
      cores[i].current_job = job;
      job->start_time = time;
      total_response_time += (time - job->arrival_time);
      return i;
    }
  }

  priqueue_offer(&job_queue, job);

  if (scheduling_scheme == PSJF || scheduling_scheme == PPRI)
  {
    job_t *running_job = NULL;
    for (int i = 0; i < num_cores; i++)
    {
      if (!cores[i].idle)
      {
        running_job = cores[i].current_job;
        break;
      }
    }

    if (running_job != NULL && ((scheduling_scheme == PSJF && job->remaining_time < running_job->remaining_time) ||
                                (scheduling_scheme == PPRI && job->priority < running_job->priority)))
    {
      priqueue_offer(&job_queue, running_job);
      cores[0].current_job = job;
      job->start_time = time;
      total_response_time += (time - job->arrival_time);
      return 0;
    }
  }

  return -1;
}

int scheduler_job_finished(int core_id, int job_number, int time)
{
  job_t *finished_job = cores[core_id].current_job;
  if (finished_job == NULL)
    return -1;

  finished_job->finish_time = time;
  total_turnaround_time += (finished_job->finish_time - finished_job->arrival_time);
  total_waiting_time += (finished_job->finish_time - finished_job->arrival_time - finished_job->burst_time);
  total_jobs++;

  free(finished_job);
  cores[core_id].idle = true;

  job_t *next_job = priqueue_poll(&job_queue);
  if (next_job != NULL)
  {
    cores[core_id].idle = false;
    cores[core_id].current_job = next_job;
    if (next_job->start_time == -1)
    {
      next_job->start_time = time;
      total_response_time += (time - next_job->arrival_time);
    }
    return next_job->job_number;
  }

  return -1;
}

int scheduler_quantum_expired(int core_id, int time)
{
  if (scheduling_scheme != RR)
    return -1;

  job_t *current_job = cores[core_id].current_job;
  if (current_job == NULL)
    return -1;

  priqueue_offer(&job_queue, current_job);
  cores[core_id].idle = true;

  job_t *next_job = priqueue_poll(&job_queue);
  if (next_job != NULL)
  {
    cores[core_id].idle = false;
    cores[core_id].current_job = next_job;
    if (next_job->start_time == -1)
    {
      next_job->start_time = time;
      total_response_time += (time - next_job->arrival_time);
    }
    return next_job->job_number;
  }

  return -1;
}

float scheduler_average_waiting_time()
{
  return (float)total_waiting_time / total_jobs;
}

float scheduler_average_turnaround_time()
{
  return (float)total_turnaround_time / total_jobs;
}

float scheduler_average_response_time()
{
  return (float)total_response_time / total_jobs;
}

void scheduler_clean_up()
{
  priqueue_destroy(&job_queue);
  free(cores);
}

void scheduler_show_queue()
{
  // Optional debugging function
}