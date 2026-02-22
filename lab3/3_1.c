#include <stdio.h>
#include <pthread.h>

#define ITERATIONS 10000000
#define NUM_THREADS 4

int counter = 0; // Shared variable
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *increment(void *arg)
{
    int thread_id = *((int *)arg);
    for (int i = 0; i < ITERATIONS; i++)
    {

        pthread_mutex_lock(&lock);

        counter++; // NOT THREAD - SAFE !

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}
int main()
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, increment, &thread_ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    printf(" Final counter value : %d \n ", counter);
    printf(" Expected value : %d \n ", NUM_THREADS * ITERATIONS);
    return 0;
}