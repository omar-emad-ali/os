/* Unit test for pcb.c */
#include <assert.h>
#include <stdio.h>
#include "../../pcb.h"

int main(void)
{
    PCB *a, *b;

    assert(pcb_table_init(3) == 0);
    a = pcb_create(1, 1, 6, 3);
    b = pcb_create(2, 3, 4, 5);
    assert(a && b && pcb_count() == 2);
    assert(pcb_get(1) == a && pcb_get(2) == b && pcb_get(9) == NULL);
    assert(pcb_create(1, 2, 2, 2) == NULL);        /* duplicate id */
    assert(pcb_create(7, 1, 0, 2) == NULL);        /* runtime 0 */
    assert(pcb_create(50, 1, 2, 2) != NULL);       /* table grows */

    pcb_add_executed(a, 2);
    assert(a->executed == 2 && a->remaining == 4);
    assert(pcb_update_waiting(a, 7) == 4);         /* 7 - 1 - 2 */

    b->finish_time = 11;                           /* arrival 3, runtime 4 */
    assert(pcb_turnaround(b) == 8 && pcb_wta(b) == 2.0);

    pcb_destroy(a);
    assert(pcb_get(1) == NULL && pcb_count() == 2);
    pcb_table_destroy();
    printf("test_pcb: all passed\n");
    return 0;
}