#ifndef KV_H
#define KV_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>

/* -------- Protocol constants (do NOT change) ---------------------------- */

#define MAX_KEY_LEN    256
#define MAX_VAL_LEN    256
#define MAX_LINE_LEN   (MAX_KEY_LEN + MAX_VAL_LEN + 64)  /* + command + ttl */

/* Response strings. Each response is one line ending in '\n'. */
#define RESP_OK        "OK\n"
#define RESP_BYE       "BYE\n"
#define RESP_NOTFOUND  "NOT_FOUND\n"

/* -------- Your types go here -------------------------------------------- */

/*
 * (Stage 1): Define your hash-table entry and bucket types.
 */ 
typedef struct kv_entry {
    char key[257];
    char value[257];
    struct kv_entry *next;
    time_t expires_at;
} kv_entry_t;

typedef struct {
    kv_entry_t **buckets;
    int num_buckets;
    
    // Statistics counters for the STATS command
    _Atomic long keys;
    _Atomic long hits;
    _Atomic long misses;
    _Atomic long puts;
    _Atomic long dels;

    pthread_rwlock_t rwlock;
    double sweeper_interval_ms;
} hash_table_t;

// Function prototypes (so other files know these functions exist)
 /* (Stage 2): Define your work-queue type (bounded FIFO of int fds).
 */
typedef struct {
    int *fds;
    int capacity;
    int count;
    int head;
    int tail;
    
    hash_table_t *table;      /* Pointer to the hash table for workers */
    pthread_mutex_t lock;   // Protects this entire structure
    pthread_cond_t not_empty; // Workers wait on this if count == 0
    pthread_cond_t not_full;  // Main thread waits on this if count == capacity
} work_queue_t;

typedef struct {
    work_queue_t *q;
    hash_table_t *table;
} thread_args_t;

/* Stage 4: Sweeper thread function */
void* sweeper_main(void *arg);

extern volatile sig_atomic_t g_shutdown;

/* -------- Function prototypes you will likely want ---------------------- */

/* Protocol / connection handling (Stage 1) */
unsigned long hash_function(const char *str);
void handle_client(int conn_fd, hash_table_t *table);        /* loop: read line, parse, reply */

/* Hash-table operations (Stage 1, made thread-safe in Stage 3) */
hash_table_t* kv_init(int num_buckets, int sweeper_ms);
void kv_destroy(hash_table_t *table);
int  kv_get(hash_table_t *table, const char *key, char *out_val);
int  kv_put(hash_table_t *table, const char *key, const char *val, int ttl);
int  kv_del(hash_table_t *table, const char *key);

#endif /* KV_H */
