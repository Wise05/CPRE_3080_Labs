/*
 * Exercise 3: The Starving Writer
 * CprE 3080 - Lab 6: Synchronization Investigation
 *
 * A shared data store accessed by multiple readers and one writer.
 * Uses a readers-writers monitor that is CORRECT (no crashes, no
 * data corruption) but UNFAIR — readers can continuously enter
 * even while the writer is waiting.
 *
 * The program runs for 5 seconds, then prints statistics.
 *
 * TASK: Observe the starvation, then modify the reader entry
 *       condition to give writers priority.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

/* ── Readers-Writers Monitor ── */

typedef struct {
    int AR;   /* Active Readers */
    int WR;   /* Waiting Readers */
    int AW;   /* Active Writers (0 or 1) */
    int WW;   /* Waiting Writers */
    pthread_mutex_t lock;
    pthread_cond_t ok_to_read;
    pthread_cond_t ok_to_write;
} RWMonitor;

static RWMonitor mon;
static volatile int running = 1;

/* Counters for statistics */
static atomic_int total_reads = 0;
static atomic_int total_writes = 0;

void rw_init(RWMonitor* m) {
    m->AR = m->WR = m->AW = m->WW = 0;
    pthread_mutex_init(&m->lock, NULL);
    pthread_cond_init(&m->ok_to_read, NULL);
    pthread_cond_init(&m->ok_to_write, NULL);
}

void rw_destroy(RWMonitor* m) {
    pthread_mutex_destroy(&m->lock);
    pthread_cond_destroy(&m->ok_to_read);
    pthread_cond_destroy(&m->ok_to_write);
}

void read_lock(RWMonitor* m) {
    pthread_mutex_lock(&m->lock);
    /*
     * CURRENT: Readers only wait if a writer is ACTIVE.
     * They do NOT wait if a writer is WAITING.
     *
     * TODO: Change this condition to also consider WW
     *       to give writers priority:
     *       while (m->AW + m->WW > 0)
     */
    while (m->AW + m->WW > 0) {
        m->WR++;
        pthread_cond_wait(&m->ok_to_read, &m->lock);
        m->WR--;
    }
    m->AR++;
    pthread_mutex_unlock(&m->lock);
}

void read_unlock(RWMonitor* m) {
    pthread_mutex_lock(&m->lock);
    m->AR--;
    if (m->AR == 0 && m->WW > 0) {
        pthread_cond_signal(&m->ok_to_write);
    }
    pthread_mutex_unlock(&m->lock);
}

void write_lock(RWMonitor* m) {
    pthread_mutex_lock(&m->lock);
    while (m->AW + m->AR > 0) {
        m->WW++;
        pthread_cond_wait(&m->ok_to_write, &m->lock);
        m->WW--;
    }
    m->AW++;
    pthread_mutex_unlock(&m->lock);
}

void write_unlock(RWMonitor* m) {
    pthread_mutex_lock(&m->lock);
    m->AW--;
    if (m->WW > 0) {
        pthread_cond_signal(&m->ok_to_write);
    } else if (m->WR > 0) {
        pthread_cond_broadcast(&m->ok_to_read);
    }
    pthread_mutex_unlock(&m->lock);
}

/* ── Threads ── */

void* reader_thread(void* arg) {
    (void)arg;

    while (running) {
        read_lock(&mon);
        /*
         * Simulate a read — short busy-wait instead of usleep
         * to avoid OS scheduler granularity hiding the effect.
         * Spins for ~50 microseconds.
         */
        volatile int spin = 0;
        for (int j = 0; j < 5000; j++) spin++;
        read_unlock(&mon);

        atomic_fetch_add(&total_reads, 1);
        /* No gap — reader immediately tries to re-enter */
    }
    return NULL;
}

void* writer_thread(void* arg) {
    (void)arg;

    while (running) {
        write_lock(&mon);
        /* Simulate a write operation */
        usleep(2000);  /* 2ms */
        write_unlock(&mon);

        atomic_fetch_add(&total_writes, 1);
        /* Small pause before next write attempt */
        usleep(1000);  /* 1ms */
    }
    return NULL;
}

/* Timer thread — stops the experiment after 5 seconds */
void* timer_thread(void* arg) {
    (void)arg;
    sleep(5);
    running = 0;
    return NULL;
}

int main(void) {
    setbuf(stdout, NULL);

    int num_readers = 20;
    pthread_t readers[20], writer, timer;

    rw_init(&mon);

    /* Start timer */
    pthread_create(&timer, NULL, timer_thread, NULL);

    /* Start writer */
    pthread_create(&writer, NULL, writer_thread, NULL);

    /* Start readers */
    for (int i = 0; i < num_readers; i++) {
        pthread_create(&readers[i], NULL, reader_thread, NULL);
    }

    /* Wait for timer to expire */
    pthread_join(timer, NULL);

    /* Wait for all threads to finish current operation */
    pthread_join(writer, NULL);
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }

    /* Print statistics */
    int reads = atomic_load(&total_reads);
    int writes = atomic_load(&total_writes);

    printf("\n=== Results (5-second run) ===\n");
    printf("Readers:                %d\n", num_readers);
    printf("Total reads completed:  %d\n", reads);
    printf("Total writes completed: %d\n", writes);
    if (writes > 0) {
        printf("Read-to-write ratio:    %.0f : 1\n",
               (double)reads / writes);
    } else {
        printf("Read-to-write ratio:    INFINITE (writer got 0 turns!)\n");
    }
    printf("==============================\n");

    rw_destroy(&mon);
    return 0;
}
