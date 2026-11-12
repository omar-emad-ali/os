/* Process Control Block. */
#ifndef PCB_H
#define PCB_H

#include <sys/types.h>

typedef enum {
    PS_NOT_ARRIVED = 0,
    PS_READY,
    PS_RUNNING,
    PS_BLOCKED,
    PS_FINISHED
} ProcState;

typedef struct PCB {
    int   id;                 /* [P3] process ID from processes.txt          */
    pid_t os_pid;             /* real Linux pid after fork (0 = not started) */
    int   arrival;            /* [P3] arrival second                         */
    int   runtime;            /* [P3] total CPU seconds needed               */
    int   original_priority;  /* 0 (highest) .. 10 (lowest)                  */
    int   effective_priority; /* used by HPF / aging comparators            */
    int   inherited_priority; /* -1 when no inheritance is active            */
    ProcState state;
    int   executed;           /* [P3] CPU seconds already executed           */
    int   remaining;          /* [P3] runtime - executed                     */
    int   waiting;            /* now - arrival - executed (kept by engine)   */
    int   start_time;         /* first second it really ran (-1 = never)     */
    int   finish_time;        /* second it finished (-1 = not yet)           */
    int   has_run;            /* 0 -> log "started", 1 -> log "resumed"      */
    int   ready_since;        /* second it last became Ready (aging)         */
    int   held_resource;      /* resource index it holds, -1 = none          */
    int   waiting_resource;   /* resource index it is blocked on, -1 = none  */
    int   heap_index;         /* [P3] position in a ready heap, -1 = not in  */
} PCB;

/* ---- PCB table API (implemented in pcb.c, P2) ---- */

/* Create the PCB table; expected_count is the number of processes. 0 on success. */
int pcb_table_init(int expected_count);

/* Free every remaining PCB and the table itself. */
void pcb_table_destroy(void);

/* Create a PCB and register it by id. Returns NULL on bad input or duplicate id. */
PCB *pcb_create(int id, int arrival, int runtime, int priority);

/* O(1) lookup by process id. Returns NULL if unknown. */
PCB *pcb_get(int id);

/* Remove a PCB from the table and free it. */
void pcb_destroy(PCB *pcb);

/* Number of PCBs currently alive. */
int pcb_count(void);

/* Add CPU seconds to executed and recompute remaining. */
void pcb_add_executed(PCB *pcb, int seconds);

/* wait = now - arrival - executed (never negative). Also stores it in pcb->waiting. */
int pcb_update_waiting(PCB *pcb, int now);

/* TA = finish_time - arrival. */
int pcb_turnaround(const PCB *pcb);

/* WTA = TA / runtime (0.0 if runtime is 0). */
double pcb_wta(const PCB *pcb);

/* Text name of a state, for debugging. */
const char *pcb_state_name(ProcState state);
#endif /* PCB_H */