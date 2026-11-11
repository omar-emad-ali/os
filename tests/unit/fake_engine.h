/* Reference single-CPU engine for tests. */
#ifndef FAKE_ENGINE_H
#define FAKE_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include "../../sched_algo.h"
#include "../../logger.h"
#include "../../stats.h"

#ifndef TMP_LOG
#define TMP_LOG  "test_engine_tmp.log"
#endif
#ifndef TMP_PERF
#define TMP_PERF "test_engine_tmp.perf"
#endif

static const char *slurp(const char *path)
{
    static char buf[16384];
    FILE *f = fopen(path, "r");
    if (!f) return "";
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static void make_pcb(PCB *p, int id, int arrival, int runtime)
{
    PCB zero = {0};
    *p = zero;
    p->id = id; p->arrival = arrival; p->runtime = runtime;
    p->remaining = runtime; p->executed = 0;
    p->state = PS_NOT_ARRIVED; p->heap_index = -1;
}

/* Run a process for n seconds. */
static void run_for(PCB *p, int n) { p->executed += n; p->remaining -= n; }

/* Fake engine. */

typedef struct { int id, arrival, runtime; } Job;

/* Run one algorithm. Jobs are sorted by arrival and ID. */
static void fake_engine(SchedAlgo *algo, const AlgoParams *prm,
                        const Job *jobs, int n)
{
    PCB *pcbs = calloc((size_t)n, sizeof(PCB));
    for (int i = 0; i < n; i++)
        make_pcb(&pcbs[i], jobs[i].id, jobs[i].arrival, jobs[i].runtime);

    algo->init(prm);
    stats_init();
    logger_open(TMP_LOG);

    PCB *running = NULL, *switching_to = NULL;
    int switch_end = 0, next_arrival = 0, finished = 0;
    int cs = algo->context_switch_time;

    for (int t = 1; finished < n && t < 100000; t++) {
        int left_cpu = 0;

        /* Account for the previous second. */
        if (running)
            run_for(running, 1);

        /* Process running events. */
        if (running && running->remaining == 0) {
            log_event(t, running, LOG_FINISHED);
            stats_add(running->arrival, running->runtime, t,
                      t - running->arrival - running->executed);
            running->state = PS_FINISHED;
            running = NULL;
            left_cpu = 1;
            finished++;
        }

        /* Add arrivals in ID order. */
        while (next_arrival < n && pcbs[next_arrival].arrival == t) {
            pcbs[next_arrival].state = PS_READY;
            algo->on_arrival(&pcbs[next_arrival]);
            next_arrival++;
        }

        /* End the context switch. */
        if (switching_to && t == switch_end) {
            running = switching_to;
            switching_to = NULL;
            log_event(t, running, running->has_run ? LOG_RESUMED : LOG_STARTED);
            running->has_run = 1;
            running->state = PS_RUNNING;
        }

        /* Handle preemption and dispatch. */
        if (running && algo->should_preempt(running, t)) {
            log_event(t, running, LOG_STOPPED);
            stats_count_preemption();
            running->state = PS_READY;
            algo->on_ready(running);
            running = NULL;
            left_cpu = 1;
        }
        if (algo->on_tick)
            algo->on_tick(t);

        if (!running && !switching_to) {
            PCB *next = algo->pick_next(t);
            if (next) {
                if (left_cpu && cs > 0) {
                    switching_to = next;
                    switch_end = t + cs;
                    stats_count_context_switch();
                } else {
                    running = next;
                    log_event(t, running, running->has_run ? LOG_RESUMED : LOG_STARTED);
                    running->has_run = 1;
                    running->state = PS_RUNNING;
                }
            }
        }
    }

    logger_close();
    stats_write(TMP_PERF);
    algo->cleanup();
    free(pcbs);
}

#endif /* FAKE_ENGINE_H */
