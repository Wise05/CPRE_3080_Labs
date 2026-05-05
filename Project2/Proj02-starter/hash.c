#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "kv.h"

unsigned long hash_function(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

hash_table_t* kv_init(int num_buckets, int sweeper_ms) {
    hash_table_t *table = malloc(sizeof(hash_table_t));
    table->num_buckets = num_buckets;
    table->buckets = calloc(num_buckets, sizeof(kv_entry_t*));
    table->keys = table->hits = table->misses = table->puts = table->dels = 0;
    table->sweeper_interval_ms = (double)sweeper_ms;
    
    if (pthread_rwlock_init(&table->rwlock, NULL) != 0) {
        perror("pthread_rwlock_init");
        free(table->buckets);
        free(table);
        return NULL;
    }
    return table;
}

int kv_put(hash_table_t *table, const char *key, const char *val, int ttl) {
    pthread_rwlock_wrlock(&table->rwlock);
    unsigned int idx = hash_function(key) % table->num_buckets;
    kv_entry_t *curr = table->buckets[idx];

    time_t expires_at = (ttl > 0) ? (time(NULL) + ttl) : 0;

    // Search for existing key
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            strncpy(curr->value, val, MAX_VAL_LEN);
            curr->expires_at = expires_at;
            atomic_fetch_add(&table->puts, 1);
            pthread_rwlock_unlock(&table->rwlock);
            return 0;
        }
        curr = curr->next;
    }

    // Key not found: Create new entry
    kv_entry_t *new_node = malloc(sizeof(kv_entry_t));
    strncpy(new_node->key, key, MAX_KEY_LEN);
    strncpy(new_node->value, val, MAX_VAL_LEN);
    new_node->expires_at = expires_at;
    new_node->next = table->buckets[idx];
    table->buckets[idx] = new_node;
    
    atomic_fetch_add(&table->keys, 1);
    atomic_fetch_add(&table->puts, 1);
    pthread_rwlock_unlock(&table->rwlock);
    return 0;
}

int kv_get(hash_table_t *table, const char *key, char *out_val) {
    pthread_rwlock_rdlock(&table->rwlock);
    
    unsigned int idx = hash_function(key) % table->num_buckets;
    kv_entry_t *curr = table->buckets[idx];
    time_t now = time(NULL);

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (curr->expires_at != 0 && now >= curr->expires_at) {
                atomic_fetch_add(&table->misses, 1);
                pthread_rwlock_unlock(&table->rwlock);
                return -1; 
            }

            strcpy(out_val, curr->value);
            atomic_fetch_add(&table->hits, 1);
            pthread_rwlock_unlock(&table->rwlock);
            return 0;
        }
        curr = curr->next;
    }

    atomic_fetch_add(&table->misses, 1);
    pthread_rwlock_unlock(&table->rwlock);
    return -1; 
}

int kv_del(hash_table_t *table, const char *key) {
    pthread_rwlock_wrlock(&table->rwlock);
    unsigned int idx = hash_function(key) % table->num_buckets;
    kv_entry_t *curr = table->buckets[idx];
    kv_entry_t *prev = NULL;

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            if (prev) prev->next = curr->next;
            else table->buckets[idx] = curr->next;

            free(curr);
            atomic_fetch_add(&table->keys, -1);
            atomic_fetch_add(&table->dels, 1);
            pthread_rwlock_unlock(&table->rwlock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_rwlock_unlock(&table->rwlock);
    return -1; // NOT_FOUND
}



void* sweeper_main(void *arg) {
    hash_table_t *table = (hash_table_t*)arg;
    struct timespec ts;
    ts.tv_sec = (time_t)(table->sweeper_interval_ms / 1000);
    ts.tv_nsec = (long)((table->sweeper_interval_ms - (ts.tv_sec * 1000)) * 1000000.0);

    while (!g_shutdown) {
        nanosleep(&ts, NULL);
        if (g_shutdown) break;
        
        pthread_rwlock_wrlock(&table->rwlock);
        time_t now = time(NULL);
        for (int i = 0; i < table->num_buckets; i++) {
            kv_entry_t **curr_ptr = &table->buckets[i];
            while (*curr_ptr) {
                kv_entry_t *entry = *curr_ptr;
                if (entry->expires_at != 0 && entry->expires_at < now) {
                    *curr_ptr = entry->next;
                    free(entry);
                    atomic_fetch_add(&table->keys, -1);
                } else {
                    curr_ptr = &entry->next;
                }
            }
        }
        pthread_rwlock_unlock(&table->rwlock);
    }
    return NULL;
}

void kv_destroy(hash_table_t *table) {
    if (!table) return;
    
    pthread_rwlock_wrlock(&table->rwlock);
    for (int i = 0; i < table->num_buckets; i++) {
        kv_entry_t *curr = table->buckets[i];
        while (curr) {
            kv_entry_t *next = curr->next;
            free(curr);
            curr = next;
        }
    }
    free(table->buckets);
    pthread_rwlock_unlock(&table->rwlock);
    pthread_rwlock_destroy(&table->rwlock);
    free(table);
}
