/* scheduler.perf writer and run counters. */
#ifndef STATS_H
#define STATS_H

#include <stdio.h>

#define PERF_FILE_NAME "scheduler.perf"

/* Set this if the utilization time origin changes. */
#ifndef STATS_TIME_ORIGIN
#define STATS_TIME_ORIGIN 0
#endif

typedef struct StatsSummary {
    int    count;            /* finished processes        */
    int    cpu_utilization;  /* percent, floored          */
    double avg_wta;          /* unrounded                 */
    int    avg_waiting;      /* floored                   */
    double std_wta;          /* unrounded, population std */
} StatsSummary;

/* Reset all counters. */
void stats_init(void);

/* Record one finished process. */
void stats_add(int arrival, int runtime, int finish, int wait);

/* Compute the current summary. */
StatsSummary stats_summary(void);

/* Write the performance summary. */
int stats_write(const char *path);

/* Run counters. */
void stats_count_context_switch(void);
void stats_count_preemption(void);
int  stats_context_switches(void);
int  stats_preemptions(void);
void stats_print_counters(FILE *out);

#endif /* STATS_H */
