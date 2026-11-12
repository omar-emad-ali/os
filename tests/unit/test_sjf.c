/* Unit test for sjf.c: shortest job first, ties by arrival then id. */
#include <assert.h>
#include <stdio.h>
#include "../../sched_algo.h"
#include "../../pcb.h"

int main(void)
{
    AlgoParams params = { 0, 1, 4 };
    PCB *p1, *p2, *p3, *p4;

    assert(pcb_table_init(4) == 0);
    p1 = pcb_create(1, 1, 6, 3);
    p2 = pcb_create(2, 2, 3, 3);
    p3 = pcb_create(3, 2, 3, 3);   /* same runtime and arrival as p2 */
    p4 = pcb_create(4, 3, 1, 3);

    assert(SJF_ALGO.init(&params) == 0);
    assert(SJF_ALGO.context_switch_time == 1);
    SJF_ALGO.on_arrival(p1);
    SJF_ALGO.on_arrival(p2);
    SJF_ALGO.on_arrival(p3);
    SJF_ALGO.on_arrival(p4);
    assert(SJF_ALGO.ready_count() == 4);
    assert(SJF_ALGO.should_preempt(p1, 5) == 0);

    assert(SJF_ALGO.pick_next(5) == p4);   /* runtime 1 */
    assert(SJF_ALGO.pick_next(5) == p2);   /* tie: smaller id */
    assert(SJF_ALGO.pick_next(5) == p3);
    assert(SJF_ALGO.pick_next(5) == p1);
    assert(SJF_ALGO.pick_next(5) == NULL);

    SJF_ALGO.cleanup();
    pcb_table_destroy();
    printf("test_sjf: all passed\n");
    return 0;
}