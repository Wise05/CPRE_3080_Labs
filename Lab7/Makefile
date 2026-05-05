CC = gcc
CFLAGS = -Wall -Wextra -g -O1

all: tlb_experiment pagesim

tlb_experiment: tlb_experiment.c
	$(CC) $(CFLAGS) -o $@ $<

pagesim: pagesim.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f tlb_experiment pagesim
