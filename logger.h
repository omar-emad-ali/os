/* scheduler.log writer. */
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

/* Open the log file and write its header. */
int logger_open(const char *path);

/* Log an event for a process. */
void log_event(int time, const PCB *pcb, LogState state);

/* Log an event from explicit values. */
void log_line(int time, int id, LogState state,
              int arrival, int total, int remain);

/* Flush and close the log file. */
void logger_close(void);

/* Return the text for a log state. */
const char *log_state_name(LogState state);

#endif /* LOGGER_H */
