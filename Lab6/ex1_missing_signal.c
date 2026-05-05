/*
 * Exercise 1: The Missing Signal
 * CprE 3080 - Lab 6: Synchronization Investigation
 *
 * A task dispatcher uses a monitor (mutex + CV) to coordinate work.
 * The dispatcher enqueues tasks; the worker waits for tasks to process.
 *
 * BUG: The dispatcher never signals the condition variable.
 *      Can the worker ever wake up?
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TASKS 5

typedef struct {
    char* tasks[MAX_TASKS];
    int count;
    pthread_mutex_t lock;
    pthread_cond_t has_task;
} TaskQueue;

static TaskQueue queue;

void queue_init(TaskQueue* q) {
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->has_task, NULL);
}

void queue_destroy(TaskQueue* q) {
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->has_task);
}

/* Dispatcher: enqueues a task into the shared queue */
void dispatch_task(TaskQueue* q, char* task) {
    pthread_mutex_lock(&q->lock);
    q->tasks[q->count] = task;
    q->count++;
    printf("[Dispatcher] Enqueued: %s (queue size: %d)\n", task, q->count);
    pthread_cond_signal(&q->has_task);
    pthread_mutex_unlock(&q->lock);
}

/* Worker: waits for a task, then processes it */
void* worker(void* arg) {
    TaskQueue* q = (TaskQueue*)arg;

    pthread_mutex_lock(&q->lock);
    while (q->count == 0) {
        printf("[Worker] Queue empty. Waiting...\n");
        pthread_cond_wait(&q->has_task, &q->lock);
    }
    q->count--;
    char* task = q->tasks[q->count];
    printf("[Worker] Processing: %s\n", task);
    pthread_mutex_unlock(&q->lock);

    return NULL;
}

int main(void) {
    setbuf(stdout, NULL);  /* Ensure output is visible even if program hangs */
    pthread_t worker_thread;
    queue_init(&queue);

    /* Start worker first - it will wait for a task */
    pthread_create(&worker_thread, NULL, worker, &queue);

    /* Small delay to ensure worker starts waiting */
    usleep(100000);  /* 100ms */

    /* Dispatcher enqueues a task */
    dispatch_task(&queue, "Compute report");

    /* Wait for worker to finish */
    pthread_join(worker_thread, NULL);

    printf("[Main] All done.\n");
    queue_destroy(&queue);
    return 0;
}
