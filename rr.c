/* Round Robin scheduling algorithm. */
#include <stddef.h>
#include "sched_algo.h"
#include "queue.h"

static Queue *ready_queue = NULL;
static int    quantum = 0;
static PCB   *current = NULL;              /* last dispatched process */
static int    executed_at_dispatch = 0;    /* executed time at dispatch */

/* Start a fresh quantum for p. */
static void start_slice(PCB *p)
{
    current = p;
    executed_at_dispatch = p ? p->executed : 0;
}

static int rr_init(const AlgoParams *params)
{
    if (!params || params->quantum <= 0)
        return -1;                         /* quantum must be positive */
    quantum = params->quantum;
    RR_ALGO.context_switch_time =
        params->context_switch_time >= 0 ? params->context_switch_time : 1;
    ready_queue = queue_create(params->process_count > 0 ? params->process_count : 0);
    current = NULL;
    executed_at_dispatch = 0;
    return ready_queue ? 0 : -1;
}

/* Add arrivals to the tail. */
static void rr_on_arrival(PCB *pcb)
{
    if (pcb)
        queue_enqueue(ready_queue, pcb);
}

/* Requeue a preempted process. */
static void rr_on_ready(PCB *pcb)
{
    if (!pcb)
        return;
    if (pcb == current)
        current = NULL;
    queue_enqueue(ready_queue, pcb);
}

/* Head of the queue is dispatched; its quantum starts now. */
static PCB *rr_pick_next(int now)
{
    (void)now;
    PCB *next = queue_dequeue(ready_queue);
    if (next)
        start_slice(next);
    return next;
}

static int rr_should_preempt(PCB *running, int now)
{
    (void)now;
    if (!running)
        return 0;
    if (running != current)                /* Keep state consistent. */
        start_slice(running);

    if (running->executed - executed_at_dispatch < quantum)
        return 0;
    if (running->remaining <= 0)
        return 0;
    if (queue_is_empty(ready_queue)) {
        start_slice(running);
        return 0;
    }
    return 1;
}

static int rr_ready_count(void)
{
    return queue_size(ready_queue);
}

static void rr_cleanup(void)
{
    queue_destroy(ready_queue);
    ready_queue = NULL;
    current = NULL;
}

SchedAlgo RR_ALGO = {
    .name = "Round Robin",
    .uses_resources = 0,
    .context_switch_time = 1,
    .init = rr_init,
    .on_arrival = rr_on_arrival,
    .on_ready = rr_on_ready,
    .pick_next = rr_pick_next,
    .should_preempt = rr_should_preempt,
    .on_tick = NULL,
    .ready_count = rr_ready_count,
    .cleanup = rr_cleanup,
};
