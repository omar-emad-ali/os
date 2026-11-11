/*
 * stats.c - scheduler.perf writer (owner: P3). See stats.h for formulas.
 */
#include <math.h>
#include "stats.h"

static int       count;
static long long sum_runtime;
static long long sum_wait;
static int       last_finish;
static double    wta_mean;   /* Welford running mean                  */
static double    wta_m2;     /* Welford sum of squared differences    */
static int       context_switches;
static int       preemptions;

void stats_init(void)
{
    count = 0;
    sum_runtime = 0;
    sum_wait = 0;
    last_finish = 0;
    wta_mean = 0.0;
    wta_m2 = 0.0;
    context_switches = 0;
    preemptions = 0;
}

void stats_add(int arrival, int runtime, int finish, int wait)
{
    if (runtime <= 0)                 /* defensive: invalid input, skip */
        return;
    int ta = finish - arrival;
    double wta = (double)ta / (double)runtime;

    count++;
    sum_runtime += runtime;
    sum_wait += wait;
    if (finish > last_finish)
        last_finish = finish;

    double delta = wta - wta_mean;
    wta_mean += delta / count;
    wta_m2 += delta * (wta - wta_mean);
}

StatsSummary stats_summary(void)
{
    StatsSummary s = {0, 0, 0.0, 0, 0.0};
    s.count = count;
    if (count == 0)
        return s;

    long long elapsed = (long long)last_finish - STATS_TIME_ORIGIN;
    if (elapsed > 0)
        s.cpu_utilization = (int)((sum_runtime * 100) / elapsed); /* exact floor */
    if (s.cpu_utilization > 100)
        s.cpu_utilization = 100;

    s.avg_wta = wta_mean;
    /* floor of the mean; wait can't be negative, but floor properly anyway */
    s.avg_waiting = (int)floor((double)sum_wait / count);
    double variance = wta_m2 / count;               /* population: / N */
    s.std_wta = variance > 0.0 ? sqrt(variance) : 0.0;
    return s;
}

int stats_write(const char *path)
{
    FILE *f = fopen(path ? path : PERF_FILE_NAME, "w");
    if (!f) {
        perror("stats: cannot open perf file");
        return -1;
    }
    StatsSummary s = stats_summary();
    fprintf(f, "CPU utilization = %d%%\n", s.cpu_utilization);
    fprintf(f, "Avg WTA = %.2f\n", s.avg_wta);
    fprintf(f, "Avg Waiting = %d\n", s.avg_waiting);
    fprintf(f, "Std WTA = %.2f\n", s.std_wta);
    fclose(f);
    return 0;
}

void stats_count_context_switch(void) { context_switches++; }
void stats_count_preemption(void)     { preemptions++; }
int  stats_context_switches(void)     { return context_switches; }
int  stats_preemptions(void)          { return preemptions; }

void stats_print_counters(FILE *out)
{
    if (!out)
        return;
    fprintf(out, "[stats] processes finished: %d, context switches: %d, preemptions: %d\n",
            count, context_switches, preemptions);
}
