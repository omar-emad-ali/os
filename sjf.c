/* Shortest Job First: non-preemptive, ready processes kept in a min-heap (P2). */
#include <stdlib.h>
#include "heap.h"
#include "sched_algo.h"

#define SJF_CONTEXT_SWITCH 1     /* project rule: 1 second for SJF */
#define SJF_MIN_HEAP_SIZE  16

static Heap *ready_heap = NULL;

/* Order: shortest runtime, then earlier arrival, then smaller id. */
static int sjf_compare(const void *a, const void *b)
{
    const PCB *p = a;
    const PCB *q = b;

    if (p->runtime != q->runtime)
        return p->runtime < q->runtime ? -1 : 1;
    if (p->arrival != q->arrival)
        return p->arrival < q->arrival ? -1 : 1;
    if (p->id != q->id)
        return p->id < q->id ? -1 : 1;
    return 0;
}

/* The heap calls this so each PCB always knows its own position. */
static void sjf_set_index(void *elem, int index)
{
    ((PCB *)elem)->heap_index = index;
}

/* Create the ready heap. Returns 0 on success, -1 on failure. */
static int sjf_init(const AlgoParams *params)
{
    int capacity = SJF_MIN_HEAP_SIZE;

    if (params && params->process_count > capacity)
        capacity = params->process_count;

    SJF_ALGO.context_switch_time = SJF_CONTEXT_SWITCH;   /* fixed, ignores user value */
    ready_heap = heap_create(capacity, sjf_compare, sjf_set_index);
    return ready_heap ? 0 : -1;
}

/* A new process arrived: mark it ready and add it to the heap. */
static void sjf_on_arrival(PCB *pcb)
{
    if (!ready_heap || !pcb)
        return;
    pcb->state = PS_READY;
    heap_insert(ready_heap, pcb);
}

/* SJF never stops a process, so this is only used for completeness. */
static void sjf_on_ready(PCB *pcb)
{
    sjf_on_arrival(pcb);
}

/* Remove and return the shortest ready process, or NULL if none is ready. */
static PCB *sjf_pick_next(int now)
{
    PCB *next;

    (void)now;
    if (!ready_heap)
        return NULL;
    next = heap_extract_min(ready_heap);
    if (next)
        next->heap_index = -1;      /* no longer in the heap */
    return next;
}

/* Non-preemptive: a running process always finishes. */
static int sjf_should_preempt(PCB *running, int now)
{
    (void)running;
    (void)now;
    return 0;
}

/* SJF has nothing to update each second. */
static void sjf_on_tick(int now)
{
    (void)now;
}

/* Number of processes waiting in the heap. */
static int sjf_ready_count(void)
{
    return ready_heap ? heap_size(ready_heap) : 0;
}

/* Free the heap (the PCBs themselves belong to the PCB table). */
static void sjf_cleanup(void)
{
    heap_destroy(ready_heap);
    ready_heap = NULL;
}

SchedAlgo SJF_ALGO = {
    .name = "SJF",
    .uses_resources = 0,
    .context_switch_time = SJF_CONTEXT_SWITCH,
    .init = sjf_init,
    .on_arrival = sjf_on_arrival,
    .on_ready = sjf_on_ready,
    .pick_next = sjf_pick_next,
    .should_preempt = sjf_should_preempt,
    .on_tick = sjf_on_tick,
    .ready_count = sjf_ready_count,
    .cleanup = sjf_cleanup
};