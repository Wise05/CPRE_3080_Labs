/**
 * Exercise 4: Performance Exploration - Lock Contention Benchmark
 * File: ex4_benchmark.c
 * 
 * This program measures how lock contention affects performance
 * as the number of threads increases.
 * 
 * Usage: ./ex4_benchmark <num_threads> <iterations_per_thread>
 * Example: ./ex4_benchmark 4 1000000
 * 
 * Compile: gcc -o ex4_benchmark ex4_benchmark.c -lpthread
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
long shared_counter = 0;

void *increment_locked(void *arg) {
    long iterations = *((long *)arg);
    
    for (long i = 0; i < iterations; i++) {
        pthread_mutex_lock(&lock);
        shared_counter++;
        pthread_mutex_unlock(&lock);
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_threads> <iterations_per_thread>\n", argv[0]);
        printf("Example: %s 4 1000000\n", argv[0]);
        return 1;
    }
    
    int num_threads = atoi(argv[1]);
    long iterations = atol(argv[2]);
    
    if (num_threads <= 0 || iterations <= 0) {
        printf("Error: threads and iterations must be positive\n");
        return 1;
    }
    
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        printf("Error: Failed to allocate memory\n");
        return 1;
    }
    
    // Start timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Create threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, increment_locked, &iterations);
    }
    
    // Wait for threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // End timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    long expected = num_threads * iterations;
    
    printf("Threads: %d, Iterations: %ld, Time: %.3f sec\n", 
           num_threads, iterations, elapsed);
    printf("Counter: %ld (expected: %ld) - %s\n", 
           shared_counter, expected,
           shared_counter == expected ? "CORRECT" : "WRONG!");
    
    free(threads);
    return 0;
}
