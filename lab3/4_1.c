#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int balance = 1000;

void *withdraw(void *arg)
{
    int amount = *((int *)arg);
    pthread_mutex_lock(&lock);

    if (balance >= amount)
    {
        printf("Withdrawing $ % d \n", amount);
        balance -= amount;
    }
    else
    {
        printf(" Insufficient funds \n ");
    }

    pthread_mutex_unlock(&lock); // unlock should not be in the conditional

    return NULL;
}

int main()
{
    pthread_t t1, t2;
    int amount1 = 600, amount2 = 600;

    pthread_create(&t1, NULL, withdraw, &amount1);
    pthread_create(&t2, NULL, withdraw, &amount2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf(" Final balance : $ % d \n ", balance);
    return 0;
}