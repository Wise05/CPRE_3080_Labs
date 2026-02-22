/**
 * Exercise 2 Bug 3: The Thread ID Mystery
 * File: ex2_bug3.c
 * 
 * This program tries to give each thread a unique ID,
 * but something goes wrong. Threads sometimes print
 * the same ID or IDs greater than expected.
 * 
 * Your task: Run it multiple times, observe the bug,
 * explain what causes it, and provide TWO different fixes.
 * 
 * Compile: gcc -o ex2_bug3 ex2_bug3.c -lpthread
 */

#include <stdio.h>
#include <pthread.h>

void *print_id(void *arg) {
    int thread_id = *((int *)arg);
    printf("Hello from thread %d\n", thread_id);
    return NULL;
}

int main() {
    pthread_t threads[5];
    
    for (int i = 0; i < 5; i++) {
        pthread_create(&threads[i], NULL, print_id, &i);
    }
    
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
