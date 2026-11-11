/* Unit tests for Round Robin. */
#include <stdlib.h>
#include "test_util.h"
#include "../../sched_algo.h"
#include "../../logger.h"
#include "../../stats.h"

#include "fake_engine.h"

/* Plug-in API tests. */

static void test_init(void)
{
    AlgoParams bad = { .quantum = 0, .context_switch_time = 1, .process_count = 3 };
    CHECK(RR_ALGO.init(&bad) == -1);           /* quantum must be > 0 */
    CHECK(RR_ALGO.init(NULL) == -1);
    AlgoParams ok = { .quantum = 2, .context_switch_time = 3, .process_count = 0 };
    CHECK(RR_ALGO.init(&ok) == 0);
    CHECK(RR_ALGO.context_switch_time == 3);
    CHECK(RR_ALGO.pick_next(1) == NULL);       /* empty queue */
    CHECK(RR_ALGO.should_preempt(NULL, 1) == 0);
    RR_ALGO.cleanup();
}

/* Arrivals stay ahead of preempted processes. */
static void test_assumption_2_and_3(void)
{
    AlgoParams prm = { .quantum = 2, .context_switch_time = 1, .process_count = 4 };
    RR_ALGO.init(&prm);
    PCB a, b, c, d;
    make_pcb(&a, 1, 1, 10); make_pcb(&b, 2, 1, 10);
    make_pcb(&c, 3, 3, 10); make_pcb(&d, 4, 4, 10);

    RR_ALGO.on_arrival(&a);
    RR_ALGO.on_arrival(&b);
    CHECK(RR_ALGO.pick_next(1) == &a);
    run_for(&a, 2);                              /* t = 3: quantum used */
    RR_ALGO.on_arrival(&c);                      /* step 2: arrival at 3 */
    CHECK(RR_ALGO.should_preempt(&a, 3) == 1);   /* step 3 */
    RR_ALGO.on_ready(&a);
    CHECK(RR_ALGO.pick_next(3) == &b);           /* switch 3 -> 4 */
    RR_ALGO.on_arrival(&d);                      /* arrives during switch */
    /* Queue order is c, a, d. */
    CHECK(RR_ALGO.ready_count() == 3);
    CHECK(RR_ALGO.pick_next(5) == &c);
    CHECK(RR_ALGO.pick_next(5) == &a);
    CHECK(RR_ALGO.pick_next(5) == &d);
    RR_ALGO.cleanup();
}

/* Quantum boundary cases. */
static void test_assumption_4_and_5(void)
{
    AlgoParams prm = { .quantum = 3, .context_switch_time = 1, .process_count = 2 };
    RR_ALGO.init(&prm);
    PCB a, b;
    make_pcb(&a, 1, 1, 7); make_pcb(&b, 2, 1, 3);

    /* An empty queue starts a new quantum. */
    RR_ALGO.on_arrival(&a);
    CHECK(RR_ALGO.pick_next(1) == &a);
    run_for(&a, 2);
    CHECK(RR_ALGO.should_preempt(&a, 3) == 0);   /* 2 < 3 */
    run_for(&a, 1);
    CHECK(RR_ALGO.should_preempt(&a, 4) == 0);   /* expired, queue empty */
    RR_ALGO.on_arrival(&b);                      /* someone arrives at 5 */
    run_for(&a, 1);
    CHECK(RR_ALGO.should_preempt(&a, 5) == 0);   /* new quantum: only 1 used */
    run_for(&a, 2);
    CHECK(RR_ALGO.should_preempt(&a, 7) == 1);   /* 3 used, b waiting */
    RR_ALGO.on_ready(&a);
    CHECK(RR_ALGO.pick_next(7) == &b);

    /* Finishing at the quantum boundary does not preempt. */
    run_for(&b, 3);
    CHECK(b.remaining == 0);
    CHECK(RR_ALGO.should_preempt(&b, 11) == 0);  /* finished, not stopped */
    RR_ALGO.cleanup();
}

/* Context-switch time does not count toward the quantum. */
static void test_quantum_ignores_switch(void)
{
    AlgoParams prm = { .quantum = 2, .context_switch_time = 5, .process_count = 2 };
    RR_ALGO.init(&prm);
    PCB a, b;
    make_pcb(&a, 1, 1, 9); make_pcb(&b, 2, 1, 9);
    RR_ALGO.on_arrival(&a); RR_ALGO.on_arrival(&b);
    CHECK(RR_ALGO.pick_next(1) == &a);
    run_for(&a, 2);
    CHECK(RR_ALGO.should_preempt(&a, 3) == 1);
    RR_ALGO.on_ready(&a);
    CHECK(RR_ALGO.pick_next(3) == &b);           /* 5 s switch, b runs at 8 */
    CHECK(RR_ALGO.should_preempt(&b, 8) == 0);   /* executed 0: nothing used */
    run_for(&b, 1);
    CHECK(RR_ALGO.should_preempt(&b, 9) == 0);
    RR_ALGO.cleanup();
}

/* Mixed scheduling case. */
static void test_engine_mixed(void)
{
    Job jobs[] = { {1, 1, 5}, {2, 2, 2}, {3, 4, 3} };
    AlgoParams prm = { .quantum = 3, .context_switch_time = 1, .process_count = 3 };
    fake_engine(&RR_ALGO, &prm, jobs, 3);
    CHECK_STR(slurp(TMP_LOG),
        "#At time x process y state arr w total z remain y wait k\n"
        "At time 1 process 1 started arr 1 total 5 remain 5 wait 0\n"
        "At time 4 process 1 stopped arr 1 total 5 remain 2 wait 0\n"
        "At time 5 process 2 started arr 2 total 2 remain 2 wait 3\n"
        "At time 7 process 2 finished arr 2 total 2 remain 0 wait 3 TA 5 WTA 2.50\n"
        "At time 8 process 3 started arr 4 total 3 remain 3 wait 4\n"
        "At time 11 process 3 finished arr 4 total 3 remain 0 wait 4 TA 7 WTA 2.33\n"
        "At time 12 process 1 resumed arr 1 total 5 remain 2 wait 8\n"
        "At time 14 process 1 finished arr 1 total 5 remain 0 wait 8 TA 13 WTA 2.60\n");
    CHECK_STR(slurp(TMP_PERF),
        "CPU utilization = 71%\n"
        "Avg WTA = 2.48\n"
        "Avg Waiting = 5\n"
        "Std WTA = 0.11\n");
    CHECK(stats_context_switches() == 3);
    CHECK(stats_preemptions() == 1);
}

/* A single process does not produce stop lines. */
static void test_engine_single(void)
{
    Job jobs[] = { {1, 1, 7} };
    AlgoParams prm = { .quantum = 3, .context_switch_time = 1, .process_count = 1 };
    fake_engine(&RR_ALGO, &prm, jobs, 1);
    CHECK_STR(slurp(TMP_LOG),
        "#At time x process y state arr w total z remain y wait k\n"
        "At time 1 process 1 started arr 1 total 7 remain 7 wait 0\n"
        "At time 8 process 1 finished arr 1 total 7 remain 0 wait 0 TA 7 WTA 1.00\n");
}

/* An idle CPU adds no switch overhead. */
static void test_engine_idle_gap(void)
{
    Job jobs[] = { {1, 1, 2}, {2, 6, 1} };
    AlgoParams prm = { .quantum = 2, .context_switch_time = 1, .process_count = 2 };
    fake_engine(&RR_ALGO, &prm, jobs, 2);
    CHECK_STR(slurp(TMP_LOG),
        "#At time x process y state arr w total z remain y wait k\n"
        "At time 1 process 1 started arr 1 total 2 remain 2 wait 0\n"
        "At time 3 process 1 finished arr 1 total 2 remain 0 wait 0 TA 2 WTA 1.00\n"
        "At time 6 process 2 started arr 6 total 1 remain 1 wait 0\n"
        "At time 7 process 2 finished arr 6 total 1 remain 0 wait 0 TA 1 WTA 1.00\n");
    CHECK(stats_context_switches() == 0);
}

/* Same-second arrivals keep ID order. */
static void test_engine_same_second(void)
{
    Job jobs[] = { {1, 1, 2}, {2, 1, 2}, {3, 1, 1} };
    AlgoParams prm = { .quantum = 1, .context_switch_time = 0, .process_count = 3 };
    fake_engine(&RR_ALGO, &prm, jobs, 3);
    CHECK_STR(slurp(TMP_LOG),
        "#At time x process y state arr w total z remain y wait k\n"
        "At time 1 process 1 started arr 1 total 2 remain 2 wait 0\n"
        "At time 2 process 1 stopped arr 1 total 2 remain 1 wait 0\n"
        "At time 2 process 2 started arr 1 total 2 remain 2 wait 1\n"
        "At time 3 process 2 stopped arr 1 total 2 remain 1 wait 1\n"
        "At time 3 process 3 started arr 1 total 1 remain 1 wait 2\n"
        "At time 4 process 3 finished arr 1 total 1 remain 0 wait 2 TA 3 WTA 3.00\n"
        "At time 4 process 1 resumed arr 1 total 2 remain 1 wait 2\n"
        "At time 5 process 1 finished arr 1 total 2 remain 0 wait 2 TA 4 WTA 2.00\n"
        "At time 5 process 2 resumed arr 1 total 2 remain 1 wait 3\n"
        "At time 6 process 2 finished arr 1 total 2 remain 0 wait 3 TA 5 WTA 2.50\n");
}

/* Stress: 1000 processes must finish, every one exactly once. */
static void test_engine_stress(void)
{
    enum { N = 1000 };
    Job *jobs = malloc(N * sizeof(Job));
    srand(7);
    int arrival = 1;
    for (int i = 0; i < N; i++) {
        if (i) arrival += rand() % 3;
        jobs[i].id = i + 1; jobs[i].arrival = arrival; jobs[i].runtime = rand() % 30 + 1;
    }
    AlgoParams prm = { .quantum = 4, .context_switch_time = 1, .process_count = N };
    fake_engine(&RR_ALGO, &prm, jobs, N);
    CHECK(stats_summary().count == N);
    free(jobs);
}

int main(void)
{
    test_init();
    test_assumption_2_and_3();
    test_assumption_4_and_5();
    test_quantum_ignores_switch();
    test_engine_mixed();
    test_engine_single();
    test_engine_idle_gap();
    test_engine_same_second();
    test_engine_stress();
    remove(TMP_LOG);
    remove(TMP_PERF);
    return TEST_SUMMARY("rr");
}
