/*
 * logger.c - scheduler.log writer (owner: P3). See logger.h for the format.
 */
#include <stdlib.h>
#include "logger.h"

#define LOG_BUFFER_SIZE (64 * 1024)

static FILE *log_file = NULL;
static char *log_buffer = NULL;

const char *log_state_name(LogState state)
{
    switch (state) {
    case LOG_STARTED:  return "started";
    case LOG_RESUMED:  return "resumed";
    case LOG_STOPPED:  return "stopped";
    case LOG_FINISHED: return "finished";
    }
    return "unknown";
}

int logger_open(const char *path)
{
    logger_close();                       /* never leak a previous file */
    log_file = fopen(path ? path : LOG_FILE_NAME, "w");
    if (!log_file) {
        perror("logger: cannot open log file");
        return -1;
    }
    /* full buffering: one write() per 64 KB instead of one per line */
    log_buffer = malloc(LOG_BUFFER_SIZE);
    if (log_buffer)
        setvbuf(log_file, log_buffer, _IOFBF, LOG_BUFFER_SIZE);
    fprintf(log_file, "#At time x process y state arr w total z remain y wait k\n");
    return 0;
}

void log_line(int time, int id, LogState state,
              int arrival, int total, int remain)
{
    if (!log_file)
        return;
    int executed = total - remain;
    int wait = time - arrival - executed;

    fprintf(log_file, "At time %d process %d %s arr %d total %d remain %d wait %d",
            time, id, log_state_name(state), arrival, total, remain, wait);

    if (state == LOG_FINISHED) {
        int ta = time - arrival;
        double wta = total > 0 ? (double)ta / (double)total : 0.0;
        fprintf(log_file, " TA %d WTA %.2f", ta, wta);
    }
    fputc('\n', log_file);
}

void log_event(int time, const PCB *pcb, LogState state)
{
    if (!pcb)
        return;
    log_line(time, pcb->id, state, pcb->arrival, pcb->runtime, pcb->remaining);
}

void logger_close(void)
{
    if (log_file) {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
    free(log_buffer);                     /* only after fclose (setvbuf rule) */
    log_buffer = NULL;
}
