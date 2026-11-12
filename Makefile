# Modules linked into the scheduler.
P2_MODULES = pcb.c sjf.c
P3_MODULES = queue.c heap.c rr.c logger.c stats.c
UNIT_DIR   = tests/unit
UNIT_FLAGS = -Wall -Wextra -g

build:
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c $(P2_MODULES) $(P3_MODULES) -o scheduler.out -lm
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out processes.txt resources.txt scheduler.log scheduler.perf
	rm -f $(UNIT_DIR)/*.out

all: clean build

run:
	./process_generator.out

# Unit tests.
unit-tests:
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_queue_heap.c queue.c heap.c -o $(UNIT_DIR)/test_queue_heap.out
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_logger_stats.c logger.c stats.c -o $(UNIT_DIR)/test_logger_stats.out -lm
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_rr.c rr.c queue.c logger.c stats.c -o $(UNIT_DIR)/test_rr.out -lm
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_doc_examples.c rr.c queue.c heap.c logger.c stats.c -o $(UNIT_DIR)/test_doc_examples.out -lm
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_pcb.c pcb.c -o $(UNIT_DIR)/test_pcb.out
	gcc $(UNIT_FLAGS) $(UNIT_DIR)/test_sjf.c sjf.c pcb.c heap.c -o $(UNIT_DIR)/test_sjf.out
	cd $(UNIT_DIR) && ./test_queue_heap.out && ./test_logger_stats.out && ./test_rr.out && ./test_doc_examples.out && ./test_pcb.out && ./test_sjf.out

# End-to-end tests.
test: build
	./run_tests.sh

.PHONY: build clean all run unit-tests test