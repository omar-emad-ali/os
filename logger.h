/*
 * logger.h - scheduler.log writer (owner: P3)
 *
 * The file is compared automatically, so the format is exact:
 *
 *   #At time x process y state arr w total z remain y wait k
 *   At time 1 process 1 started arr 1 total 6 remain 6 wait 0
 *   At time 4 process 1 stopped arr 1 total 6 remain 3 wait 0
 *   At time 9 process 1 finished arr 1 total 6 remain 0 wait 2 TA 8 WTA 1.33
 *
 * Rules:
 *   - wait   = time - arrival - executed      (executed = total - remain)
 *   - TA     = time - arrival                 (finished lines only)
 *   - WTA    = TA / total, printed with %.2f  (finished lines only)
 *   - started/resumed: log at the second the context switch ENDS
 *     stopped/finished: log at the second the process LEAVES the CPU
 *     (choosing that second is the engine's job, P2)
 *
 * Do NOT include headers.h here: it defines globals and would break linking.
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include "pcb.h"

#define LOG_FILE_NAME "scheduler.log"

typedef enum {
    LOG_STARTED = 0,
    LOG_RESUMED,
    LOG_STOPPED,
    LOG_FINISHED
} LogState;

/* Open (truncate) the log file and write the header line.
 * path == NULL uses scheduler.log. Returns 0 on success, -1 on failure. */
int logger_open(const char *path);

/* Log one event for pcb at second `time`. The PCB must already hold the
 * up-to-date executed / remaining values for that second. */
void log_event(int time, const PCB *pcb, LogState state);

/* Same as log_event but with plain values (used by tests and by anyone
 * who does not have a PCB at hand). */
void log_line(int time, int id, LogState state,
              int arrival, int total, int remain);

/* Flush and close. Safe to call twice or without logger_open. */
void logger_close(void);

/* "started" / "resumed" / "stopped" / "finished" */
const char *log_state_name(LogState state);

#endif /* LOGGER_H */
