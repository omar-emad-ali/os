/* Generic FIFO queue implementation. */
#include <stdlib.h>
#include "queue.h"

#define QUEUE_DEFAULT_CAPACITY 16

Queue *queue_create(int initial_capacity)
{
    Queue *q = malloc(sizeof(Queue));
    if (!q)
        return NULL;
    if (initial_capacity <= 0)
        initial_capacity = QUEUE_DEFAULT_CAPACITY;
    q->items = malloc(sizeof(void *) * (size_t)initial_capacity);
    if (!q->items) {
        free(q);
        return NULL;
    }
    q->capacity = initial_capacity;
    q->head = 0;
    q->size = 0;
    return q;
}

void queue_destroy(Queue *q)
{
    if (!q)
        return;
    free(q->items);
    free(q);
}

/* Grow the buffer and reset the head index. */
static int queue_grow(Queue *q)
{
    int new_capacity = q->capacity * 2;
    void **new_items = malloc(sizeof(void *) * (size_t)new_capacity);
    if (!new_items)
        return -1;
    for (int i = 0; i < q->size; i++)
        new_items[i] = q->items[(q->head + i) % q->capacity];
    free(q->items);
    q->items = new_items;
    q->capacity = new_capacity;
    q->head = 0;
    return 0;
}

int queue_enqueue(Queue *q, void *item)
{
    if (!q)
        return -1;
    if (q->size == q->capacity && queue_grow(q) != 0)
        return -1;
    int tail = (q->head + q->size) % q->capacity;
    q->items[tail] = item;
    q->size++;
    return 0;
}

void *queue_dequeue(Queue *q)
{
    if (!q || q->size == 0)
        return NULL;
    void *item = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->size--;
    return item;
}

void *queue_peek(const Queue *q)
{
    if (!q || q->size == 0)
        return NULL;
    return q->items[q->head];
}

int queue_size(const Queue *q)
{
    return q ? q->size : 0;
}

int queue_is_empty(const Queue *q)
{
    return queue_size(q) == 0;
}

int queue_remove(Queue *q, const void *item)
{
    if (!q)
        return 0;
    for (int i = 0; i < q->size; i++) {
        if (q->items[(q->head + i) % q->capacity] == item) {
            /* Keep the remaining items in order. */
            for (int j = i; j < q->size - 1; j++)
                q->items[(q->head + j) % q->capacity] =
                    q->items[(q->head + j + 1) % q->capacity];
            q->size--;
            return 1;
        }
    }
    return 0;
}
