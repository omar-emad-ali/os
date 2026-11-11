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

#endif /* PCB_H */
