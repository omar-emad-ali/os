/*
 * heap.h - Generic binary min-heap (owner: P3)
 *
 * Elements are void*; the user supplies:
 *   - cmp(a, b)          : < 0 if a must come out before b, > 0 if after,
 *                          0 if equal (always break ties in cmp so the order
 *                          is deterministic, e.g. by arrival then ID).
 *   - set_index(e, idx)  : OPTIONAL. Called every time an element moves, so
 *                          the element can remember its position (e.g. the
 *                          PCB's heap_index field). idx = -1 when removed.
 *                          Pass NULL if you never need heap_update/heap_remove_at.
 *
 * Complexity: insert, extract_min, update, remove_at -> O(log n); peek O(1).
 *
 * Changing priority of an element already inside (priority inheritance,
 * aging): change the field the comparator reads, then call
 *      heap_update(h, pcb->heap_index);
 * Removing an arbitrary element: heap_remove_at(h, pcb->heap_index).
 */
#ifndef HEAP_H
#define HEAP_H

typedef int  (*heap_cmp_fn)(const void *a, const void *b);
typedef void (*heap_index_fn)(void *elem, int index);

typedef struct Heap {
    void **data;
    int size;
    int capacity;
    heap_cmp_fn cmp;
    heap_index_fn set_index;
} Heap;

/* Create an empty heap. initial_capacity <= 0 picks a small default. */
Heap *heap_create(int initial_capacity, heap_cmp_fn cmp, heap_index_fn set_index);

/* Free the heap array (not the elements). Safe with NULL. */
void heap_destroy(Heap *h);

/* Insert elem. Returns 0 on success, -1 on allocation failure. */
int heap_insert(Heap *h, void *elem);

/* Remove and return the minimum element, or NULL if empty. */
void *heap_extract_min(Heap *h);

/* Return the minimum element without removing it, or NULL if empty. */
void *heap_peek(const Heap *h);

/* Restore heap order after the element at index changed its key
 * (works whether the key got better or worse). Returns 0, or -1 on bad index. */
int heap_update(Heap *h, int index);

/* Remove and return the element at index, or NULL on bad index. */
void *heap_remove_at(Heap *h, int index);

/* Linear search for elem; returns its index or -1. Useful when no
 * set_index callback is used. */
int heap_find(const Heap *h, const void *elem);

int heap_size(const Heap *h);
int heap_is_empty(const Heap *h);

/* Debug / test helper: returns 1 if every parent compares <= its children,
 * 0 otherwise. O(n). */
int heap_is_valid(const Heap *h);

#endif /* HEAP_H */
