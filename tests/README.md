# Tests (owner: P3)

## Quick start

```
make unit-tests          # P3 modules alone: queue, heap, logger, stats, RR (~1 s)
./run_tests.sh           # every end-to-end case in tests/cases/ (needs the full program)
./run_tests.sh rr_mixed_q3_cs1          # one case
./run_tests.sh --random 20 42 3         # 20 random processes, seed 42, algorithm 3
./run_tests.sh --random 20 42 2 4 1     # same with RR, quantum 4, context switch 1
TIMEOUT=120 ./run_tests.sh              # change the per-case limit (default 600 s)
```

The clock ticks once per real second, so a case lasts about as long as its
last finish time.

## What every end-to-end run checks

- the program exits by itself (no crash, no timeout)
- no message queue / shared memory / semaphore is left (`ipcs`)
- no process of the simulation is still alive (no orphans)
- `scheduler.log`: exact header, every line in the exact format, every
  process finished exactly once
- `scheduler.perf`: exactly four lines in the exact format
- if `expected.log` / `expected.perf` exist: `diff` must be empty

## Case folder layout

```
tests/cases/<name>/
    processes.txt     tab separated, as in the project document
    resources.txt     optional
    input             what is typed to process_generator.out
    expected.log      optional
    expected.perf     optional
```

`input` must follow P1's prompt order: algorithm number, then for RR the
quantum and the context-switch time. Example for RR, quantum 3, switch 1:

```
2
3
1
```

## Current cases

| Case | Algorithm | Expected output | What it covers |
|---|---|---|---|
| doc_sjf | SJF | yes (from the PDF) | document SJF example |
| doc_rr_q3_cs1 | RR q=3 cs=1 | yes (from the PDF) | document RR example |
| doc_hpf_resources | HPF | yes (from the PDF) | blocking on x, inheritance, hand-over |
| doc_hpf_output_example | HPF | yes (from the PDF) | the Output files section example |
| doc_hpf_aging | HPF aging (bonus) | yes (from the PDF) | aging and demotion |
| rr_mixed_q3_cs1 | RR q=3 cs=1 | yes (hand-computed) | quantum expiry, assumptions 4 and 7, resume |
| rr_single_q3_cs1 | RR q=3 cs=1 | yes | assumption 5: no stop line when alone |
| rr_idle_gap_q2_cs1 | RR q=2 cs=1 | yes | idle CPU, start with no switch overhead |
| rr_same_second_q1_cs0 | RR q=1 cs=0 | yes | assumption 6 (ID order), zero switch time |
| edge_sjf_runtime1_same_second | SJF | sanity only | comments, blank line, runtime 1, same second |
| edge_sjf_long_idle | SJF | sanity only | 17 idle seconds |
| edge_hpf_equal_priorities | HPF | sanity only | equal priority never preempts |
| edge_hpf_three_waiters | HPF | sanity only | three waiters on r1, hand-over, inheritance |
| edge_hpf_release_equals_capture | HPF | sanity only | release and capture in the same second |
| edge_aging_starvation | HPF aging | sanity only | low priority process waits > 20 s |

## To do when the other parts exist

1. The `doc_*` expected logs start with the `#At time x ...` header line
   (as in the Output files section); the PDF's algorithm examples omit it.
   If the TA says the header must not be written, delete it from the
   logger and from these files together.
2. For each `edge_*` case: once its owner has checked the output by hand,
   run `./run_tests.sh --bless <case>` to save it as the expected output.
3. Valgrind on the scheduler: `valgrind --leak-check=full ./scheduler.out ...`
   with the arguments P1 passes it.
4. Bonus: run every case with algorithm 3 and 4 and give P4 the perf files
   plus the counter line `[stats] ... context switches ... preemptions`.
5. Final check: clone the repo into an empty folder, `make`, `./run_tests.sh`.
