#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
long shared_counter = 0;

void *increment_locked(void *arg)
{
    long iterations = *((long *)arg);

    for (long i = 0; i < iterations; i++)
    {
        __sync_fetch_and_add(&shared_counter, 1);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf(" Usage : % s < num_threads > < iterations_per_thread >\n", argv[0]);
        return 1;
    }

    int num_threads = atoi(argv[1]);
    long iterations = atol(argv[2]);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < num_threads; i++)
    {
        pthread_create(&threads[i], NULL, increment_locked, &iterations);
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf(" Threads : %d , Time : %.3f sec , Counter : %ld \n",
           num_threads, elapsed, shared_counter);

    free(threads);
    return 0;
}