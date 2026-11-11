/*
 * queue.h - Generic FIFO queue (owner: P3)
 *
 * A growable circular buffer of void* items.
 *   - enqueue / dequeue / peek : O(1) (enqueue is amortised O(1) when it grows)
 *   - remove of an arbitrary item: O(n) (rarely needed, e.g. defensive cleanup)
 *
 * The queue never owns the items: destroying it frees only the buffer.
 * Used by Round Robin (rr.c) and as a general helper.
 */
#ifndef QUEUE_H
#define QUEUE_H

typedef struct Queue {
    void **items;   /* circular buffer                        */
    int capacity;   /* allocated slots                        */
    int head;       /* index of the first (oldest) item       */
    int size;       /* number of stored items                 */
} Queue;

/* Create an empty queue. initial_capacity <= 0 picks a small default.
 * Returns NULL if memory allocation fails. */
Queue *queue_create(int initial_capacity);

/* Free the queue buffer (not the items). Safe to call with NULL. */
void queue_destroy(Queue *q);

/* Append item at the tail. Returns 0 on success, -1 on failure. */
int queue_enqueue(Queue *q, void *item);

/* Remove and return the head item, or NULL if the queue is empty. */
void *queue_dequeue(Queue *q);

/* Return the head item without removing it, or NULL if empty. */
void *queue_peek(const Queue *q);

/* Number of items currently stored (0 for a NULL queue). */
int queue_size(const Queue *q);

/* 1 if the queue is empty (or NULL), 0 otherwise. */
int queue_is_empty(const Queue *q);

/* Remove the first occurrence of item, keeping the order of the others.
 * Returns 1 if it was found and removed, 0 otherwise. */
int queue_remove(Queue *q, const void *item);

#endif /* QUEUE_H */
