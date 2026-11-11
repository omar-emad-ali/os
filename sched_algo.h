/*
 * sched_algo.h - Plug-in interface every scheduling algorithm implements
 *
 * >>> OWNER: P2.  DRAFT written by P3 for the kick-off so rr.c compiles.
 * >>> Agree on it in the meeting; after that, changes need all four.
 *
 * The engine (scheduler.c) never knows which algorithm runs. Per simulated
 * second it calls, in this order:
 *   step 1  running process's own events (release, capture, finish)
 *           - if the running process finished/blocked: CPU becomes free
 *   step 2  for each arrival of this second, in ID order:  on_arrival(pcb)
 *   step 3  if a process is running:
 *               should_preempt(running, now)
 *               -> true : engine stops it, then on_ready(running)
 *           on_tick(now)  (aging, may be NULL)
 *           if the CPU is free: pick_next(now) -> dispatch (with or
 *           without context switch, decided by the engine)
 *
 * on_ready is ALSO called when a blocked process is unblocked (HPF).
 * The engine must call on_ready for a preempted process AFTER the
 * arrivals of the same second (RR assumption 2) - the order above does it.
 */
#ifndef SCHED_ALGO_H
#define SCHED_ALGO_H

#include "pcb.h"

typedef struct AlgoParams {
    int quantum;              /* RR only, > 0                               */
    int context_switch_time;  /* seconds: 1 for SJF/HPF, user value for RR  */
    int process_count;        /* total processes (lets algos pre-size)      */
} AlgoParams;

typedef struct SchedAlgo {
    const char *name;
    int uses_resources;       /* 1 for HPF / HPF-aging, 0 for SJF / RR      */
    int context_switch_time;  /* filled by init() from AlgoParams           */

    /* Allocate the ready structure. Return 0 on success, -1 on failure. */
    int  (*init)(const AlgoParams *params);
    /* A new process arrived (step 2). */
    void (*on_arrival)(PCB *pcb);
    /* A process became ready again: preempted / quantum expired / unblocked. */
    void (*on_ready)(PCB *pcb);
    /* Remove and return the process to dispatch next, or NULL if none.
     * The returned process is considered dispatched from now on. */
    PCB *(*pick_next)(int now);
    /* Should the running process leave the CPU now? (step 3) */
    int  (*should_preempt)(PCB *running, int now);
    /* Called once per second (aging). May be NULL. */
    void (*on_tick)(int now);
    /* Number of processes waiting in the ready structure. */
    int  (*ready_count)(void);
    /* Free everything. */
    void (*cleanup)(void);
} SchedAlgo;

/* One instance per algorithm, defined in its own .c file. */
extern SchedAlgo SJF_ALGO;        /* sjf.c       (P2) */
extern SchedAlgo RR_ALGO;         /* rr.c        (P3) */
extern SchedAlgo HPF_ALGO;        /* hpf.c       (P4) */
extern SchedAlgo HPF_AGING_ALGO;  /* hpf_aging.c (P4) */

#endif /* SCHED_ALGO_H */
