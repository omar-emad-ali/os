/* Generic FIFO queue. */
#ifndef QUEUE_H
#define QUEUE_H

typedef struct Queue {
    void **items;   /* circular buffer                        */
    int capacity;   /* allocated slots                        */
    int head;       /* index of the first (oldest) item       */
    int size;       /* number of stored items                 */
} Queue;

/* Create an empty queue. */
Queue *queue_create(int initial_capacity);

/* Free the queue buffer, not its items. */
void queue_destroy(Queue *q);

/* Append item at the tail. Returns 0 on success, -1 on failure. */
int queue_enqueue(Queue *q, void *item);

/* Remove and return the head item, or NULL if the queue is empty. */
void *queue_dequeue(Queue *q);

/* Return the head item without removing it, or NULL if empty. */
void *queue_peek(const Queue *q);

/* Return the number of stored items. */
int queue_size(const Queue *q);

/* Return whether the queue is empty. */
int queue_is_empty(const Queue *q);

/* Remove the first matching item. */
int queue_remove(Queue *q, const void *item);

#endif /* QUEUE_H */
