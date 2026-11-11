/* Generic binary min-heap implementation. */
#include <stdlib.h>
#include "heap.h"

#define HEAP_DEFAULT_CAPACITY 16

/* Store an element and update its index. */
static void heap_place(Heap *h, int i, void *elem)
{
    h->data[i] = elem;
    if (h->set_index)
        h->set_index(elem, i);
}

/* Move an element toward the root. */
static int sift_up(Heap *h, int i)
{
    void *elem = h->data[i];
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->cmp(elem, h->data[parent]) >= 0)
            break;
        heap_place(h, i, h->data[parent]);
        i = parent;
    }
    heap_place(h, i, elem);
    return i;
}

/* Move an element toward the leaves. */
static int sift_down(Heap *h, int i)
{
    void *elem = h->data[i];
    for (;;) {
        int left = 2 * i + 1;
        if (left >= h->size)
            break;
        int smallest = left;
        int right = left + 1;
        if (right < h->size && h->cmp(h->data[right], h->data[left]) < 0)
            smallest = right;
        if (h->cmp(h->data[smallest], elem) >= 0)
            break;
        heap_place(h, i, h->data[smallest]);
        i = smallest;
    }
    heap_place(h, i, elem);
    return i;
}

Heap *heap_create(int initial_capacity, heap_cmp_fn cmp, heap_index_fn set_index)
{
    if (!cmp)
        return NULL;
    Heap *h = malloc(sizeof(Heap));
    if (!h)
        return NULL;
    if (initial_capacity <= 0)
        initial_capacity = HEAP_DEFAULT_CAPACITY;
    h->data = malloc(sizeof(void *) * (size_t)initial_capacity);
    if (!h->data) {
        free(h);
        return NULL;
    }
    h->size = 0;
    h->capacity = initial_capacity;
    h->cmp = cmp;
    h->set_index = set_index;
    return h;
}

void heap_destroy(Heap *h)
{
    if (!h)
        return;
    free(h->data);
    free(h);
}

int heap_insert(Heap *h, void *elem)
{
    if (!h)
        return -1;
    if (h->size == h->capacity) {
        int new_capacity = h->capacity * 2;
        void **bigger = realloc(h->data, sizeof(void *) * (size_t)new_capacity);
        if (!bigger)
            return -1;
        h->data = bigger;
        h->capacity = new_capacity;
    }
    h->data[h->size] = elem;
    h->size++;
    sift_up(h, h->size - 1);
    return 0;
}

void *heap_remove_at(Heap *h, int index)
{
    if (!h || index < 0 || index >= h->size)
        return NULL;
    void *removed = h->data[index];
    h->size--;
    if (index != h->size) {
        /* Replace the removed element and restore heap order. */
        h->data[index] = h->data[h->size];
        if (sift_up(h, index) == index)
            sift_down(h, index);
    }
    if (h->set_index)
        h->set_index(removed, -1);
    return removed;
}

void *heap_extract_min(Heap *h)
{
    return heap_remove_at(h, 0);
}

void *heap_peek(const Heap *h)
{
    if (!h || h->size == 0)
        return NULL;
    return h->data[0];
}

int heap_update(Heap *h, int index)
{
    if (!h || index < 0 || index >= h->size)
        return -1;
    if (sift_up(h, index) == index)
        sift_down(h, index);
    return 0;
}

int heap_find(const Heap *h, const void *elem)
{
    if (!h)
        return -1;
    for (int i = 0; i < h->size; i++)
        if (h->data[i] == elem)
            return i;
    return -1;
}

int heap_size(const Heap *h)
{
    return h ? h->size : 0;
}

int heap_is_empty(const Heap *h)
{
    return heap_size(h) == 0;
}

int heap_is_valid(const Heap *h)
{
    if (!h)
        return 1;
    for (int i = 1; i < h->size; i++)
        if (h->cmp(h->data[(i - 1) / 2], h->data[i]) > 0)
            return 0;
    return 1;
}
