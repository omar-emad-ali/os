/*
 * test_doc_examples.c - P3's modules against the project document examples.
 *
 * The expected files in tests/cases/doc_* are transcribed from the PDF.
 *  1. logger + stats: for every doc example, rebuild every log line and the
 *     perf file from the numbers in the expected log; both must match
 *     exactly (checks the wait formula, WTA rounding, util/avg/std rules).
 *  2. RR end to end: fake engine + rr.c on the RR example.
 *  3. SJF end to end: fake engine + a TEST-ONLY SJF plug-in built on heap.c
 *     (P2 writes the real sjf.c; this proves the heap and the engine order).
 *
 * Run from tests/unit (the Makefile does this).
 */
#include <stdlib.h>
#include "test_util.h"
#include "../../heap.h"
#include "fake_engine.h"

#define CASES "../cases/"
#define REBUILT_LOG  "test_doc_rebuilt.log"
#define REBUILT_PERF "test_doc_rebuilt.perf"

/* Copy of a file into a heap string (slurp() returns a shared buffer). */
static char *read_all(const char *path)
{
    const char *s = slurp(path);
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    memcpy(copy, s, n);
    return copy;
}

static LogState parse_state(const char *s)
{
    if (strcmp(s, "started") == 0) return LOG_STARTED;
    if (strcmp(s, "resumed") == 0) return LOG_RESUMED;
    if (strcmp(s, "stopped") == 0) return LOG_STOPPED;
    return LOG_FINISHED;
}

/* Part 1: rebuild the log and perf of one doc case from its expected log. */
static void check_formats(const char *name)
{
    char path[256], line[256];
    snprintf(path, sizeof path, CASES "%s/expected.log", name);
    FILE *in = fopen(path, "r");
    CHECK(in != NULL);
    if (!in) return;

    logger_open(REBUILT_LOG);
    stats_init();
    while (fgets(line, sizeof line, in)) {
        int t, id, arr, total, remain, wait;
        char state[16];
        if (line[0] == '#') continue;
        if (sscanf(line, "At time %d process %d %15s arr %d total %d remain %d wait %d",
                   &t, &id, state, &arr, &total, &remain, &wait) != 7) {
            CHECK(!"unparsable line in expected.log");
            continue;
        }
        LogState st = parse_state(state);
        log_line(t, id, st, arr, total, remain);
        if (st == LOG_FINISHED)
            stats_add(arr, total, t, wait);
    }
    fclose(in);
    logger_close();
    stats_write(REBUILT_PERF);

    char *expected_log = read_all(path);
    CHECK_STR(slurp(REBUILT_LOG), expected_log);
    free(expected_log);

    snprintf(path, sizeof path, CASES "%s/expected.perf", name);
    char *expected_perf = read_all(path);
    CHECK_STR(slurp(REBUILT_PERF), expected_perf);
    free(expected_perf);
}

/* Read processes.txt of a case into jobs[] (tab separated, # comments). */
static int load_jobs(const char *name, Job *jobs, int max)
{
    char path[256], line[256];
    snprintf(path, sizeof path, CASES "%s/processes.txt", name);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int n = 0, priority;
    while (n < max && fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        if (sscanf(line, "%d\t%d\t%d\t%d", &jobs[n].id, &jobs[n].arrival,
                   &jobs[n].runtime, &priority) == 4)
            n++;
    }
    fclose(f);
    return n;
}

/* Run the fake engine on a case and diff with its expected files. */
static void check_engine(const char *name, SchedAlgo *algo, int quantum, int cs)
{
    Job jobs[64];
    int n = load_jobs(name, jobs, 64);
    CHECK(n > 0);
    AlgoParams prm = { .quantum = quantum, .context_switch_time = cs, .process_count = n };
    fake_engine(algo, &prm, jobs, n);

    char path[256];
    snprintf(path, sizeof path, CASES "%s/expected.log", name);
    char *exp = read_all(path);
    CHECK_STR(slurp(TMP_LOG), exp);
    free(exp);
    snprintf(path, sizeof path, CASES "%s/expected.perf", name);
    exp = read_all(path);
    CHECK_STR(slurp(TMP_PERF), exp);
    free(exp);
}

/* ---- TEST-ONLY SJF plug-in (non-preemptive; ties: arrival, then ID) ---- */
static Heap *sjf_heap;
static int sjf_cmp(const void *a, const void *b)
{
    const PCB *x = a, *y = b;
    if (x->runtime != y->runtime) return x->runtime - y->runtime;
    if (x->arrival != y->arrival) return x->arrival - y->arrival;
    return x->id - y->id;
}
static void sjf_set_index(void *e, int i) { ((PCB *)e)->heap_index = i; }
static int  sjf_init(const AlgoParams *p)
{
    sjf_heap = heap_create(p->process_count, sjf_cmp, sjf_set_index);
    return sjf_heap ? 0 : -1;
}
static void sjf_add(PCB *p) { heap_insert(sjf_heap, p); }
static PCB *sjf_pick(int now) { (void)now; return heap_extract_min(sjf_heap); }
static int  sjf_never(PCB *p, int now) { (void)p; (void)now; return 0; }
static int  sjf_count(void) { return heap_size(sjf_heap); }
static void sjf_cleanup(void) { heap_destroy(sjf_heap); sjf_heap = NULL; }
static SchedAlgo TEST_SJF = {
    .name = "SJF (test only)", .uses_resources = 0, .context_switch_time = 1,
    .init = sjf_init, .on_arrival = sjf_add, .on_ready = sjf_add,
    .pick_next = sjf_pick, .should_preempt = sjf_never, .on_tick = NULL,
    .ready_count = sjf_count, .cleanup = sjf_cleanup,
};

int main(void)
{
    const char *cases[] = { "doc_sjf", "doc_rr_q3_cs1", "doc_hpf_resources",
                            "doc_hpf_output_example", "doc_hpf_aging" };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
        check_formats(cases[i]);

    check_engine("doc_rr_q3_cs1", &RR_ALGO, 3, 1);
    check_engine("doc_sjf", &TEST_SJF, 0, 1);

    remove(REBUILT_LOG); remove(REBUILT_PERF);
    remove(TMP_LOG); remove(TMP_PERF);
    return TEST_SUMMARY("doc examples");
}
