/* Process Control Block and PCB table (P2). */
#include <stdio.h>
#include <stdlib.h>
#include "pcb.h"

#define PCB_MAX_ID      100000   /* reject absurd ids so the table stays small */
#define PCB_MIN_CAP     16

static PCB **pcb_table = NULL;   /* pcb_table[id] -> PCB or NULL */
static int pcb_table_capacity = 0;
static int pcb_live_count = 0;

/* Create the table. Frees any older table first. Returns 0 on success, -1 on failure. */
int pcb_table_init(int expected_count)
{
    int capacity = expected_count + 1;      /* ids normally run 1..N */

    pcb_table_destroy();
    if (capacity < PCB_MIN_CAP)
        capacity = PCB_MIN_CAP;

    pcb_table = calloc(capacity, sizeof(PCB *));
    if (!pcb_table) {
        perror("pcb: cannot allocate table");
        return -1;
    }
    pcb_table_capacity = capacity;
    pcb_live_count = 0;
    return 0;
}

/* Free every PCB still alive, then the table. Safe to call twice. */
void pcb_table_destroy(void)
{
    int i;

    if (!pcb_table)
        return;
    for (i = 0; i < pcb_table_capacity; i++)
        free(pcb_table[i]);
    free(pcb_table);
    pcb_table = NULL;
    pcb_table_capacity = 0;
    pcb_live_count = 0;
}

/* Grow the table so that index id fits. Returns 0 on success, -1 on failure. */
static int pcb_table_grow(int id)
{
    int new_capacity = pcb_table_capacity;
    PCB **bigger;
    int i;

    while (new_capacity <= id)
        new_capacity *= 2;

    bigger = realloc(pcb_table, new_capacity * sizeof(PCB *));
    if (!bigger)
        return -1;
    for (i = pcb_table_capacity; i < new_capacity; i++)
        bigger[i] = NULL;               /* new slots start empty */
    pcb_table = bigger;
    pcb_table_capacity = new_capacity;
    return 0;
}

/* Build a PCB in the NOT_ARRIVED state and register it under its id. */
PCB *pcb_create(int id, int arrival, int runtime, int priority)
{
    PCB *pcb;

    if (!pcb_table || id < 0 || id > PCB_MAX_ID)
        return NULL;
    if (arrival < 0 || runtime <= 0 || priority < 0 || priority > 10)
        return NULL;
    if (id >= pcb_table_capacity && pcb_table_grow(id) != 0)
        return NULL;
    if (pcb_table[id] != NULL)
        return NULL;                    /* duplicate id */

    pcb = calloc(1, sizeof(PCB));
    if (!pcb)
        return NULL;

    pcb->id = id;
    pcb->os_pid = 0;                    /* not forked yet */
    pcb->arrival = arrival;
    pcb->runtime = runtime;
    pcb->original_priority = priority;
    pcb->effective_priority = priority;
    pcb->inherited_priority = -1;       /* no inheritance */
    pcb->state = PS_NOT_ARRIVED;
    pcb->executed = 0;
    pcb->remaining = runtime;
    pcb->waiting = 0;
    pcb->start_time = -1;
    pcb->finish_time = -1;
    pcb->has_run = 0;
    pcb->ready_since = arrival;
    pcb->held_resource = -1;
    pcb->waiting_resource = -1;
    pcb->heap_index = -1;

    pcb_table[id] = pcb;
    pcb_live_count++;
    return pcb;
}

/* Return the PCB for id in O(1), or NULL if it does not exist. */
PCB *pcb_get(int id)
{
    if (!pcb_table || id < 0 || id >= pcb_table_capacity)
        return NULL;
    return pcb_table[id];
}

/* Unregister a PCB and free it. Does nothing for NULL. */
void pcb_destroy(PCB *pcb)
{
    if (!pcb)
        return;
    if (pcb_table && pcb->id >= 0 && pcb->id < pcb_table_capacity
        && pcb_table[pcb->id] == pcb) {
        pcb_table[pcb->id] = NULL;
        pcb_live_count--;
    }
    free(pcb);
}

/* How many PCBs are alive right now. */
int pcb_count(void)
{
    return pcb_live_count;
}

/* Add executed seconds (never past runtime) and keep remaining consistent. */
void pcb_add_executed(PCB *pcb, int seconds)
{
    if (!pcb || seconds <= 0)
        return;
    pcb->executed += seconds;
    if (pcb->executed > pcb->runtime)
        pcb->executed = pcb->runtime;
    pcb->remaining = pcb->runtime - pcb->executed;
}

/* wait = now - arrival - executed. Context-switch and blocked time count as waiting. */
int pcb_update_waiting(PCB *pcb, int now)
{
    int wait;

    if (!pcb)
        return 0;
    wait = now - pcb->arrival - pcb->executed;
    if (wait < 0)
        wait = 0;
    pcb->waiting = wait;
    return wait;
}

/* Turnaround time = finish - arrival. */
int pcb_turnaround(const PCB *pcb)
{
    if (!pcb)
        return 0;
    return pcb->finish_time - pcb->arrival;
}

/* Weighted turnaround = TA / runtime, guarded against runtime 0. */
double pcb_wta(const PCB *pcb)
{
    if (!pcb || pcb->runtime <= 0)
        return 0.0;
    return (double)pcb_turnaround(pcb) / pcb->runtime;
}

/* Readable name of a state. */
const char *pcb_state_name(ProcState state)
{
    switch (state) {
    case PS_NOT_ARRIVED: return "not arrived";
    case PS_READY:       return "ready";
    case PS_RUNNING:     return "running";
    case PS_BLOCKED:     return "blocked";
    case PS_FINISHED:    return "finished";
    }
    return "unknown";
}