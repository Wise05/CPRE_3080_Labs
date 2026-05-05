/*
 * tlb_experiment.c - TLB Capacity Measurement Tool
 * CprE 308/3080 - Lab 7: Memory Management
 * 
 * Usage: ./tlb_experiment <num_pages> <num_trials>
 * 
 * This program measures the average time to access memory
 * as a function of the number of pages touched, revealing
 * the TLB capacity of your system.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define PAGE_SIZE 4096  // 4 KB pages (standard on x86)

/*
 * Returns the difference between two timespecs in nanoseconds.
 */
long long timespec_diff_ns(struct timespec *start, struct timespec *end) {
    return (long long)(end->tv_sec - start->tv_sec) * 1000000000LL
         + (long long)(end->tv_nsec - start->tv_nsec);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <num_pages> <num_trials> [stride]\n", argv[0]);
        fprintf(stderr, "  num_pages:  number of distinct elements to access (e.g., 4 to 512)\n");
        fprintf(stderr, "  num_trials: number of times to repeat the access loop (e.g., 10000)\n");
        fprintf(stderr, "  stride:     bytes between accesses (default: 4096 = one page)\n");
        return 1;
    }

    int num_pages = atoi(argv[1]);
    int num_trials = atoi(argv[2]);
    int stride = (argc >= 4) ? atoi(argv[3]) : PAGE_SIZE;

    if (num_pages <= 0 || num_trials <= 0 || stride <= 0) {
        fprintf(stderr, "Error: all arguments must be positive.\n");
        return 1;
    }

    // Allocate enough memory to span num_pages * stride bytes
    long array_size = (long)num_pages * stride;
    volatile char *array = (volatile char *)malloc(array_size);
    if (!array) {
        fprintf(stderr, "Error: failed to allocate %ld bytes.\n", array_size);
        return 1;
    }

    // Touch every element once to ensure pages are mapped (avoid cold-start faults)
    for (int i = 0; i < num_pages; i++) {
        array[(long)i * stride] = (char)i;
    }

    // Measure: access elements at the given stride, repeated num_trials times
    struct timespec start, end;
    int dummy = 0;  // Prevent compiler from optimizing away reads

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int trial = 0; trial < num_trials; trial++) {
        for (int page = 0; page < num_pages; page++) {
            dummy += array[(long)page * stride];
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long long total_ns = timespec_diff_ns(&start, &end);
    long long total_accesses = (long long)num_pages * num_trials;
    double ns_per_access = (double)total_ns / total_accesses;

    printf("Pages: %4d | Stride: %5d | Trials: %d | Total accesses: %lld | "
           "Avg time/access: %.2f ns\n",
           num_pages, stride, num_trials, total_accesses, ns_per_access);

    // Use dummy to prevent optimization
    if (dummy == -999999) printf("never\n");

    free((void *)array);
    return 0;
}
