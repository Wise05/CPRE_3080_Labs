CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TIMEOUT = 5

all: ex1 ex2 ex3

ex1: ex1_missing_signal.c
	$(CC) $(CFLAGS) -o ex1 ex1_missing_signal.c

ex2: ex2_lost_wakeup.c
	$(CC) $(CFLAGS) -o ex2 ex2_lost_wakeup.c

ex3: ex3_starving_writer.c
	$(CC) $(CFLAGS) -o ex3 ex3_starving_writer.c

# Run targets with timeout protection for exercises that may hang
run-ex1:
	@timeout $(TIMEOUT) ./ex1 || echo "*** Program timed out after $(TIMEOUT) seconds (likely waiting indefinitely) ***"

run-ex2:
	@timeout $(TIMEOUT) ./ex2 || echo "*** Program timed out after $(TIMEOUT) seconds (likely waiting indefinitely) ***"

run-ex3:
	@./ex3

clean:
	rm -f ex1 ex2 ex3

.PHONY: all clean run-ex1 run-ex2 run-ex3
