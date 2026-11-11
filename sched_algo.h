/* Interface implemented by each scheduling algorithm. */
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

    /* Allocate the ready structure. */
    int  (*init)(const AlgoParams *params);
    /* Add a new arrival. */
    void (*on_arrival)(PCB *pcb);
    /* Add a process that became ready. */
    void (*on_ready)(PCB *pcb);
    /* Select the next process, or NULL. */
    PCB *(*pick_next)(int now);
    /* Return whether the running process should leave the CPU. */
    int  (*should_preempt)(PCB *running, int now);
    /* Update the algorithm once per second. */
    void (*on_tick)(int now);
    /* Return the ready process count. */
    int  (*ready_count)(void);
    /* Release algorithm state. */
    void (*cleanup)(void);
} SchedAlgo;

/* One instance per algorithm, defined in its own .c file. */
extern SchedAlgo SJF_ALGO;        /* sjf.c       (P2) */
extern SchedAlgo RR_ALGO;         /* rr.c        (P3) */
extern SchedAlgo HPF_ALGO;        /* hpf.c       (P4) */
extern SchedAlgo HPF_AGING_ALGO;  /* hpf_aging.c (P4) */

#endif /* SCHED_ALGO_H */
