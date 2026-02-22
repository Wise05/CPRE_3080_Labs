/**
 * Exercise 2 Bug 1: The Missing Unlock
 * File: ex2_bug1.c
 * 
 * This program has a synchronization bug that can cause deadlock.
 * Your task: Find it, fix it, and explain what causes the problem.
 * 
 * Compile: gcc -o ex2_bug1 ex2_bug1.c -lpthread
 */

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int balance = 1000;

void *withdraw(void *arg) {
    int amount = *((int *)arg);
    
    pthread_mutex_lock(&lock);
    
    if (balance >= amount) {
        printf("Withdrawing $%d\n", amount);
        balance -= amount;
        pthread_mutex_unlock(&lock);
    } else {
        printf("Insufficient funds\n");
    }
    // BUG: What happens to the lock if we reach here?
    
    return NULL;
}

int main() {
    pthread_t t1, t2;
    int amount1 = 600, amount2 = 600;
    
    pthread_create(&t1, NULL, withdraw, &amount1);
    pthread_create(&t2, NULL, withdraw, &amount2);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("Final balance: $%d\n", balance);
    return 0;
}
