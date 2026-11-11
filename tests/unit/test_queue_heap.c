/*
 * test_queue_heap.c - Unit tests for queue.c and heap.c (P3)
 */
#include <stdlib.h>
#include "test_util.h"
#include "../../queue.h"
#include "../../heap.h"
#include "../../pcb.h"

/* ---------- queue ---------- */
static void test_queue(void)
{
    int v[100];
    for (int i = 0; i < 100; i++) v[i] = i;

    Queue *q = queue_create(2);   /* tiny capacity -> forces growth */
    CHECK(q != NULL);
    CHECK(queue_is_empty(q));
    CHECK(queue_dequeue(q) == NULL);
    CHECK(queue_peek(q) == NULL);

    /* wrap around before growing: push 2, pop 1, push more */
    queue_enqueue(q, &v[0]);
    queue_enqueue(q, &v[1]);
    CHECK(queue_dequeue(q) == &v[0]);
    for (int i = 2; i < 100; i++) queue_enqueue(q, &v[i]);
    CHECK(queue_size(q) == 99);
    CHECK(queue_peek(q) == &v[1]);

    /* remove from the middle keeps order */
    CHECK(queue_remove(q, &v[50]) == 1);
    CHECK(queue_remove(q, &v[50]) == 0);
    int ok = 1;
    for (int i = 1; i < 100; i++) {
        if (i == 50) continue;
        if (queue_dequeue(q) != &v[i]) ok = 0;
    }
    CHECK(ok);
    CHECK(queue_is_empty(q));
    queue_destroy(q);
    queue_destroy(NULL);          /* must not crash */
    CHECK(queue_size(NULL) == 0);
}

/* ---------- heap with PCBs (SJF-like comparator) ---------- */
static int cmp_runtime(const void *a, const void *b)
{
    const PCB *x = a, *y = b;
    if (x->runtime != y->runtime) return x->runtime - y->runtime;
    if (x->arrival != y->arrival) return x->arrival - y->arrival;
    return x->id - y->id;
}
static int cmp_priority(const void *a, const void *b)
{
    const PCB *x = a, *y = b;
    if (x->effective_priority != y->effective_priority)
        return x->effective_priority - y->effective_priority;
    return x->id - y->id;
}
static void set_idx(void *e, int i) { ((PCB *)e)->heap_index = i; }

/* every element's stored index must match its real position */
static int indices_consistent(const Heap *h)
{
    for (int i = 0; i < h->size; i++)
        if (((PCB *)h->data[i])->heap_index != i) return 0;
    return 1;
}

static void test_heap_basic(void)
{
    PCB p[5] = {0};
    int runtimes[5] = {5, 3, 3, 9, 1};
    int arrivals[5] = {1, 2, 1, 1, 4};
    for (int i = 0; i < 5; i++) {
        p[i].id = i + 1; p[i].runtime = runtimes[i];
        p[i].arrival = arrivals[i]; p[i].heap_index = -1;
    }
    Heap *h = heap_create(1, cmp_runtime, set_idx);
    CHECK(heap_extract_min(h) == NULL);
    for (int i = 0; i < 5; i++) heap_insert(h, &p[i]);
    CHECK(heap_is_valid(h));
    CHECK(indices_consistent(h));
    /* order: runtime 1 (P5), runtime 3 arr 1 (P3), runtime 3 arr 2 (P2), 5, 9 */
    int expected[5] = {5, 3, 2, 1, 4};
    int ok = 1;
    for (int i = 0; i < 5; i++) {
        PCB *x = heap_extract_min(h);
        if (!x || x->id != expected[i]) ok = 0;
        if (x && x->heap_index != -1) ok = 0;
    }
    CHECK(ok);
    CHECK(heap_is_empty(h));
    heap_destroy(h);
}

/* priority change and arbitrary removal (inheritance / aging) */
static void test_heap_update_remove(void)
{
    enum { N = 200 };
    PCB *p = calloc(N, sizeof(PCB));
    Heap *h = heap_create(4, cmp_priority, set_idx);
    srand(42);
    for (int i = 0; i < N; i++) {
        p[i].id = i + 1;
        p[i].effective_priority = rand() % 11;
        heap_insert(h, &p[i]);
    }
    CHECK(heap_is_valid(h) && indices_consistent(h));

    /* random priority changes, both directions */
    int ok = 1;
    for (int k = 0; k < 1000; k++) {
        PCB *x = &p[rand() % N];
        if (x->heap_index < 0) continue;
        x->effective_priority = rand() % 11;
        heap_update(h, x->heap_index);
        if (!heap_is_valid(h) || !indices_consistent(h)) ok = 0;
    }
    CHECK(ok);

    /* inheritance case: a low-priority holder jumps to the top */
    PCB *holder = &p[N - 1];
    holder->effective_priority = -1;           /* better than anything */
    heap_update(h, holder->heap_index);
    CHECK(heap_peek(h) == holder);

    /* arbitrary removals */
    ok = 1;
    for (int i = 0; i < N; i += 3) {
        if (p[i].heap_index < 0) continue;
        PCB *r = heap_remove_at(h, p[i].heap_index);
        if (r != &p[i] || r->heap_index != -1) ok = 0;
        if (!heap_is_valid(h) || !indices_consistent(h)) ok = 0;
    }
    CHECK(ok);
    CHECK(heap_remove_at(h, -1) == NULL);
    CHECK(heap_remove_at(h, h->size) == NULL);
    CHECK(heap_update(h, h->size) == -1);

    /* drain: non-decreasing priorities, ties by smaller ID */
    PCB *prev = NULL; ok = 1;
    while (!heap_is_empty(h)) {
        PCB *x = heap_extract_min(h);
        if (prev && cmp_priority(prev, x) > 0) ok = 0;
        prev = x;
    }
    CHECK(ok);
    heap_destroy(h);
    free(p);
}

int main(void)
{
    test_queue();
    test_heap_basic();
    test_heap_update_remove();
    return TEST_SUMMARY("queue/heap");
}
