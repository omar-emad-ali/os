/*
 * rr.c - Round Robin scheduling algorithm (owner: P3)
 *
 * Plugs into the engine through sched_algo.h (see the per-second order
 * there). Ready structure: FIFO queue (queue.c).
 *
 * The quantum is counted in EXECUTED CPU seconds since the process was last
 * dispatched (pcb->executed - executed_at_dispatch). Context-switch seconds
 * and waiting seconds therefore never eat the quantum.
 *
 * The seven assumptions of the project document and where each one lives:
 *  (1) a new arrival goes to the END of the queue ............ rr_on_arrival
 *  (2) quantum expiry + arrival in the same second: the arrival is queued
 *      first, then the preempted process ...................... engine order:
 *      step 2 (on_arrival) runs before step 3 (should_preempt -> on_ready)
 *  (3) a process arriving during a context switch (from its start up to and
 *      including the second it ends) is behind the preempted process:
 *      the preempted process was enqueued when the switch began, so every
 *      later arrival lands behind it ............................ rr_on_ready
 *  (4) finishing exactly when the quantum ends = finished, not stopped:
 *      finish is handled in step 1, before should_preempt is asked; also
 *      guarded here (remaining <= 0 never preempts) ...... rr_should_preempt
 *  (5) quantum expires and the queue is empty (after this second's
 *      arrivals): keep running a new quantum, no stop, no switch, no log
 *      line ............................................. rr_should_preempt
 *  (6) same-second arrivals are queued by ID ......... engine calls on_arrival
 *      in ID order; rr_on_arrival just appends
 *  (7) a process that finishes before its quantum triggers the context
 *      switch immediately ............ engine: pick_next right after finish
 *
 * Engine contract (P2): when should_preempt(running, t) is called at second
 * t, running->executed must already include every second it ran before t.
 */
#include <stddef.h>
#include "sched_algo.h"
#include "queue.h"

static Queue *ready_queue = NULL;
static int    quantum = 0;
static PCB   *current = NULL;              /* process dispatched last      */
static int    executed_at_dispatch = 0;    /* its executed time at dispatch */

/* Start a fresh quantum for p. */
static void start_slice(PCB *p)
{
    current = p;
    executed_at_dispatch = p ? p->executed : 0;
}

static int rr_init(const AlgoParams *params)
{
    if (!params || params->quantum <= 0)
        return -1;                         /* quantum must be > 0 */
    quantum = params->quantum;
    RR_ALGO.context_switch_time =
        params->context_switch_time >= 0 ? params->context_switch_time : 1;
    ready_queue = queue_create(params->process_count > 0 ? params->process_count : 0);
    current = NULL;
    executed_at_dispatch = 0;
    return ready_queue ? 0 : -1;
}

/* Assumptions 1 and 6: append at the tail (engine passes arrivals by ID). */
static void rr_on_arrival(PCB *pcb)
{
    if (pcb)
        queue_enqueue(ready_queue, pcb);
}

/* Preempted (quantum expired): back to the tail. Assumptions 2 and 3. */
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
    if (running != current)                /* defensive: engine dispatched it */
        start_slice(running);              /* without pick_next               */

    if (running->executed - executed_at_dispatch < quantum)
        return 0;                          /* quantum not used up yet         */
    if (running->remaining <= 0)
        return 0;                          /* assumption 4: it finishes       */
    if (queue_is_empty(ready_queue)) {
        start_slice(running);              /* assumption 5: new quantum,      */
        return 0;                          /* no stop, no switch, no log line */
    }
    return 1;                              /* engine stops it, calls on_ready */
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
