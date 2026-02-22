#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *print_id(void *arg)

{

    int thread_id = *((int *)arg);

    printf(" Hello from thread % d \n ", thread_id);

    free(arg);

    return NULL;
}

int main()

{

    pthread_t threads[5];

    for (int i = 0; i < 5; i++)

    {
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&threads[i], NULL, print_id, arg);
    }

    for (int i = 0; i < 5; i++)

    {

        pthread_join(threads[i], NULL);
    }

    return 0;
}