/**
 * Exercise 2 Bug 2: The Wrong Granularity
 * File: ex2_bug2.c
 * 
 * This program is technically correct (no race condition),
 * but performs terribly due to improper lock granularity.
 * 
 * Your task: Identify the performance problem and fix it
 * without introducing race conditions.
 * 
 * Compile: gcc -o ex2_bug2 ex2_bug2.c -lpthread
 * Time it: time ./ex2_bug2
 */

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *expensive_work(void *arg) {
    int id = *((int *)arg);
    
    pthread_mutex_lock(&lock);
    
    // Expensive computation (100 million iterations)
    long sum = 0;
    for (long i = 0; i < 100000000; i++) {
        sum += i;
    }
    
    printf("Thread %d: sum = %ld\n", id, sum);
    
    pthread_mutex_unlock(&lock);
    
    return NULL;
}

int main() {
    pthread_t threads[4];
    int ids[4];
    
    for (int i = 0; i < 4; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, expensive_work, &ids[i]);
    }
    
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
