#include <stdio.h>
#include <pthread.h>

void *expensive_work(void *arg)
{
    int id = *((int *)arg);

    // Expensive computation (100 million iterations )
    long sum = 0;
    for (long i = 0; i < 100000000; i++)
    {
        sum += i;
    }

    printf(" Thread % d : sum = % ld \n ", id, sum);

    return NULL;
}

int main()
{
    pthread_t threads[4];
    int ids[4];

    for (int i = 0; i < 4; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, expensive_work, &ids[i]);
    }

    for (int i = 0; i < 4; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}