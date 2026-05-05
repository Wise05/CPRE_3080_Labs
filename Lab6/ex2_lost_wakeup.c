/*
 * Exercise 2: The Lost Wakeup
 * CprE 3080 - Lab 6: Synchronization Investigation
 *
 * A logging system where a producer thread generates log entries
 * and a consumer thread processes them using a condition variable.
 *
 * BUG: The consumer checks the condition OUTSIDE the lock, then
 *      acquires the lock and calls cond_wait WITHOUT re-checking.
 *      Between the outside check and the wait, the producer can
 *      signal — but the consumer is NOT on the wait queue yet,
 *      so the signal is LOST. The consumer then waits forever.
 *
 * FIX: Use the standard Mesa monitor pattern — check the condition
 *      INSIDE the lock, in a while loop around cond_wait.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int log_ready;          /* 1 if a log entry is waiting */
    int log_id;
    pthread_mutex_t lock;
    pthread_cond_t has_log;
} LogQueue;

static LogQueue lq;

void lq_init(LogQueue* q) {
    q->log_ready = 0;
    q->log_id = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->has_log, NULL);
}

void lq_destroy(LogQueue* q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->has_log);
}

/* Producer: generates log entries (correctly synchronized) */
void* producer(void* arg) {
    LogQueue* q = (LogQueue*)arg;

    for (int i = 1; i <= 5; i++) {
        /* Variable delay between entries */
        usleep((rand() % 40000) + 10000);  /* 10-50ms */

        pthread_mutex_lock(&q->lock);
        q->log_id = i;
        q->log_ready = 1;
        printf("[Producer] Generated log entry #%d\n", i);
        pthread_cond_signal(&q->has_log);
        pthread_mutex_unlock(&q->lock);
    }
    return NULL;
}

/*
 * Consumer: processes log entries (BUGGY)
 *
 * The bug has TWO parts that work together:
 *   1. Condition is checked OUTSIDE the lock
 *   2. After acquiring the lock, the consumer does NOT re-check
 *      — it goes straight to cond_wait
 *
 * The race:
 *   1. Consumer reads log_ready == 0        (no lock held)
 *   2. Context switch / delay occurs
 *   3. Producer sets log_ready = 1 and signals
 *      (consumer is NOT on wait queue — signal is LOST)
 *   4. Consumer acquires lock, does NOT re-check, calls
 *      cond_wait — sleeps forever, even though log_ready == 1
 *
 * CORRECT version (Mesa monitor pattern):
 *   pthread_mutex_lock(&q->lock);
 *   while (q->log_ready == 0) {
 *       pthread_cond_wait(&q->has_log, &q->lock);
 *   }
 *   printf("[Consumer] Processing log entry #%d\n", q->log_id);
 *   q->log_ready = 0;
 *   pthread_mutex_unlock(&q->lock);
 */
void* consumer(void* arg) {
    LogQueue* q = (LogQueue*)arg;

    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&q->lock);
        while (q->log_ready == 0) {
            printf("[Consumer] No entry. Waiting...\n");
            pthread_cond_wait(&q->has_log, &q->lock);
        }

        printf("[Consumer] Processing log entry #%d\n", q->log_id);
        q->log_ready = 0;
        pthread_mutex_unlock(&q->lock);
    }
    return NULL;
}

int main(void) {
    setbuf(stdout, NULL);
    srand(time(NULL));

    pthread_t prod, cons;
    lq_init(&lq);

    /* Start BOTH threads nearly simultaneously */
    pthread_create(&prod, NULL, producer, &lq);
    pthread_create(&cons, NULL, consumer, &lq);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    printf("[Main] All log entries processed.\n");
    lq_destroy(&lq);
    return 0;
}
