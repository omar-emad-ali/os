/*
 * stats.h - scheduler.perf writer and run counters (owner: P3)
 *
 * scheduler.perf has EXACTLY four lines:
 *   CPU utilization = 85%
 *   Avg WTA = 1.34
 *   Avg Waiting = 1
 *   Std WTA = 0.34
 *
 * Formulas (per finished process):
 *   TA  = finish - arrival
 *   WTA = TA / runtime          (unrounded inside, %.2f when printed)
 *   wait = given by the engine  (now - arrival - executed at finish time)
 * Totals:
 *   CPU utilization = floor(100 * sum(runtime) / last finish time)   [integer]
 *   Avg WTA         = mean(WTA),                rounded to 2 decimals
 *   Avg Waiting     = floor(mean(wait))                              [integer]
 *   Std WTA         = population std (divide by N), rounded to 2 decimals
 *
 * Memory: O(1). Nothing is stored per process; Welford's method keeps the
 * running mean and variance of WTA.
 *
 * Context switches / preemptions are counted here but are printed only by
 * stats_print_counters() (console or debug file), never in .perf or .log.
 */
#ifndef STATS_H
#define STATS_H

#include <stdio.h>

#define PERF_FILE_NAME "scheduler.perf"

/* If the team's document measures utilization from a time origin other
 * than 0 (e.g. "elapsed = last_finish - 1"), change this ONE value. */
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

/* Reset everything (call once at scheduler start). */
void stats_init(void);

/* Record one finished process. */
void stats_add(int arrival, int runtime, int finish, int wait);

/* Compute the summary of what was recorded so far. */
StatsSummary stats_summary(void);

/* Write the four lines. path == NULL uses scheduler.perf.
 * Returns 0 on success, -1 on failure. Safe with zero processes. */
int stats_write(const char *path);

/* Run counters (for the report / bonus evaluation, not for .perf). */
void stats_count_context_switch(void);
void stats_count_preemption(void);
int  stats_context_switches(void);
int  stats_preemptions(void);
void stats_print_counters(FILE *out);

#endif /* STATS_H */
