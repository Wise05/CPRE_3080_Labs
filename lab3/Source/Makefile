# CprE 308 Lab 3 Makefile
# Compiles all exercise programs

CC = gcc
CFLAGS = -lpthread -O0 -Wall
PROGRAMS = ex1_race ex2_bug1 ex2_bug2 ex2_bug3 ex3_benchmark

all: $(PROGRAMS)

ex1_race: ex1_race.c
	$(CC) -o ex1_race ex1_race.c $(CFLAGS)

ex2_bug1: ex2_bug1.c
	$(CC) -o ex2_bug1 ex2_bug1.c $(CFLAGS)

ex2_bug2: ex2_bug2.c
	$(CC) -o ex2_bug2 ex2_bug2.c $(CFLAGS)

ex2_bug3: ex2_bug3.c
	$(CC) -o ex2_bug3 ex2_bug3.c $(CFLAGS)

ex4_benchmark: ex3_benchmark.c
	$(CC) -o ex3_benchmark ex3_benchmark.c $(CFLAGS)

clean:
	rm -f $(PROGRAMS)

test-race:
	@echo "Running race condition test 10 times..."
	@for i in 1 2 3 4 5 6 7 8 9 10; do ./ex1_race; done

test-bug1:
	@echo "Testing bug1 (may hang - press Ctrl+C if it does)..."
	@timeout 5 ./ex2_bug1 || echo "Program timed out (likely deadlock)"

test-bug2:
	@echo "Testing bug2 performance..."
	@time ./ex2_bug2

test-bug3:
	@echo "Testing bug3 multiple times..."
	@for i in 1 2 3 4 5; do ./ex2_bug3; echo "---"; done

test-benchmark:
	@echo "Running benchmark with different thread counts..."
	@./ex3_benchmark 1 1000000
	@./ex3_benchmark 2 1000000
	@./ex3_benchmark 4 1000000
	@./ex3_benchmark 8 1000000

.PHONY: all clean test-race test-bug1 test-bug2 test-bug3 test-benchmark
