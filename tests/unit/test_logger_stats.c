/* Logger and statistics tests. */
#include <stdlib.h>
#include "test_util.h"
#include "../../logger.h"
#include "../../stats.h"

#define TMP_LOG  "test_tmp.log"
#define TMP_PERF "test_tmp.perf"

/* Read a small file into a buffer. */
static const char *slurp(const char *path)
{
    static char buf[8192];
    FILE *f = fopen(path, "r");
    if (!f) return "";
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void test_logger_format(void)
{
    CHECK(logger_open(TMP_LOG) == 0);

    /* P1: arr 1, total 6, runs 1..4, stopped at 4, resumed at 6 (1 s switch
     * + 1 s someone else), finishes at 9 -> wait 2, TA 8, WTA 1.33 */
    PCB p = {0};
    p.id = 1; p.arrival = 1; p.runtime = 6;
    p.remaining = 6; log_event(1, &p, LOG_STARTED);
    p.remaining = 3; log_event(4, &p, LOG_STOPPED);
    p.remaining = 3; log_event(6, &p, LOG_RESUMED);
    p.remaining = 0; log_event(9, &p, LOG_FINISHED);

    /* WTA formatting. */
    log_line(9,  2, LOG_FINISHED, 1, 3, 0);    /* TA 8 / 3  = 2.666.. -> 2.67 */
    log_line(5,  3, LOG_FINISHED, 1, 4, 0);    /* TA 4 / 4  = 1       -> 1.00 */
    log_line(14, 4, LOG_FINISHED, 1, 8, 0);    /* TA 13/ 8  = 1.625   -> 1.62 (printf tie) */
    log_line(5,  5, LOG_FINISHED, 1, 6, 2);    /* defensive: remain>0 still prints */
    log_event(3, NULL, LOG_STARTED);           /* NULL pcb is ignored */
    logger_close();
    logger_close();                            /* double close is safe */

    CHECK_STR(slurp(TMP_LOG),
        "#At time x process y state arr w total z remain y wait k\n"
        "At time 1 process 1 started arr 1 total 6 remain 6 wait 0\n"
        "At time 4 process 1 stopped arr 1 total 6 remain 3 wait 0\n"
        "At time 6 process 1 resumed arr 1 total 6 remain 3 wait 2\n"
        "At time 9 process 1 finished arr 1 total 6 remain 0 wait 2 TA 8 WTA 1.33\n"
        "At time 9 process 2 finished arr 1 total 3 remain 0 wait 5 TA 8 WTA 2.67\n"
        "At time 5 process 3 finished arr 1 total 4 remain 0 wait 0 TA 4 WTA 1.00\n"
        "At time 14 process 4 finished arr 1 total 8 remain 0 wait 5 TA 13 WTA 1.62\n"
        "At time 5 process 5 finished arr 1 total 6 remain 2 wait 0 TA 4 WTA 0.67\n");

    /* Logging before open is ignored. */
    log_line(1, 1, LOG_STARTED, 1, 1, 1);
    remove(TMP_LOG);
}

static void test_stats_example(void)
{
    stats_init();
    /*            arr run finish wait     TA  WTA      */
    stats_add(1, 6, 7, 0);            /*  6   1.0000   */
    stats_add(3, 3, 11, 5);           /*  8   2.6667   */
    stats_add(4, 4, 14, 6);           /* 10   2.5000   */
    /* util = floor(100*13/14) = 92 ; avg WTA = 2.0556 -> 2.06
     * avg wait = floor(11/3) = 3 ; std = sqrt(0.561728) = 0.7495 -> 0.75 */
    CHECK(stats_write(TMP_PERF) == 0);
    CHECK_STR(slurp(TMP_PERF),
        "CPU utilization = 92%\n"
        "Avg WTA = 2.06\n"
        "Avg Waiting = 3\n"
        "Std WTA = 0.75\n");

    StatsSummary s = stats_summary();
    CHECK(s.count == 3);
    CHECK(s.cpu_utilization == 92);
    remove(TMP_PERF);
}

static void test_stats_edges(void)
{
    /* Zero processes still produce four lines. */
    stats_init();
    CHECK(stats_write(TMP_PERF) == 0);
    CHECK_STR(slurp(TMP_PERF),
        "CPU utilization = 0%\n"
        "Avg WTA = 0.00\n"
        "Avg Waiting = 0\n"
        "Std WTA = 0.00\n");

    /* Identical WTAs produce zero deviation. */
    stats_init();
    stats_add(1, 5, 6, 0);
    stats_add(6, 5, 11, 0);
    StatsSummary s = stats_summary();
    CHECK(s.cpu_utilization == 90);       /* 10 / 11 -> 90.9 -> 90 (floor) */
    CHECK(s.std_wta == 0.0);

    /* Utilization is floored. */
    stats_init();
    stats_add(0, 789, 1000, 211);
    CHECK(stats_summary().cpu_utilization == 78);

    /* Ignore zero runtime. */
    stats_init();
    stats_add(1, 0, 1, 0);
    CHECK(stats_summary().count == 0);

    /* Counters. */
    stats_init();
    stats_count_context_switch();
    stats_count_context_switch();
    stats_count_preemption();
    CHECK(stats_context_switches() == 2);
    CHECK(stats_preemptions() == 1);
    remove(TMP_PERF);
}

/* WTA statistics use unrounded values. */
static void test_unrounded_mean(void)
{
    stats_init();
    stats_add(0, 3, 5, 0);    /* 5/3 = 1.6667 */
    stats_add(0, 3, 5, 0);
    stats_add(0, 3, 4, 0);    /* 4/3 = 1.3333 */
    /* mean = 1.5556 -> 1.56 ; rounded-first mean = (1.67+1.67+1.33)/3 = 1.5567 -> 1.56
     * std = sqrt(((0.1111^2)*2 + 0.2222^2)/3) = 0.1571 -> 0.16 */
    StatsSummary s = stats_summary();
    char buf[64];
    snprintf(buf, sizeof buf, "%.2f %.2f", s.avg_wta, s.std_wta);
    CHECK_STR(buf, "1.56 0.16");
}

int main(void)
{
    test_logger_format();
    test_stats_example();
    test_stats_edges();
    test_unrounded_mean();
    return TEST_SUMMARY("logger/stats");
}
