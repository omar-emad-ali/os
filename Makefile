# Owner: P1. P3 added P3_MODULES to the scheduler line and the test targets.
# Every recipe line must start with a TAB.

# P3 modules linked into the scheduler (P2/P4 add sjf.c pcb.c hpf.c ... here)
P3_MODULES = queue.c heap.c rr.c logger.c stats.c
UNIT_DIR   = tests/unit
UNIT_FLAGS = -Wall -Wextra -g

build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c $(P3_MODULES) -o scheduler.out -lm
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out processes.txt resources.txt scheduler.log scheduler.perf
	rm -f $(UNIT_DIR)/*.out

all: clean build

run:
	./process_generator.out

# ---- P3: unit tests (no clock, no IPC, run in a second) ----
unit-tests:
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_queue_heap.c queue.c heap.c -o $(UNIT_DIR)/test_queue_heap.out
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_logger_stats.c logger.c stats.c -o $(UNIT_DIR)/test_logger_stats.out -lm
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_rr.c rr.c queue.c logger.c stats.c -o $(UNIT_DIR)/test_rr.out -lm
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_doc_examples.c rr.c queue.c heap.c logger.c stats.c -o $(UNIT_DIR)/test_doc_examples.out -lm
	cd $(UNIT_DIR) && ./test_queue_heap.out && ./test_logger_stats.out && ./test_rr.out && ./test_doc_examples.out

# ---- P3: end-to-end tests (needs the full program working) ----
test: build
	./run_tests.sh

.PHONY: build clean all run unit-tests test
