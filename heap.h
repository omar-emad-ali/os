/* Generic binary min-heap. */
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

/* Create an empty heap. */
Heap *heap_create(int initial_capacity, heap_cmp_fn cmp, heap_index_fn set_index);

/* Free the heap array, not its elements. */
void heap_destroy(Heap *h);

/* Insert elem. Returns 0 on success, -1 on allocation failure. */
int heap_insert(Heap *h, void *elem);

/* Remove and return the minimum element, or NULL if empty. */
void *heap_extract_min(Heap *h);

/* Return the minimum element without removing it, or NULL if empty. */
void *heap_peek(const Heap *h);

/* Restore heap order after an element changes its key. */
int heap_update(Heap *h, int index);

/* Remove and return the element at index, or NULL on bad index. */
void *heap_remove_at(Heap *h, int index);

/* Find an element by pointer. */
int heap_find(const Heap *h, const void *elem);

int heap_size(const Heap *h);
int heap_is_empty(const Heap *h);

/* Check heap order. */
int heap_is_valid(const Heap *h);

#endif /* HEAP_H */
