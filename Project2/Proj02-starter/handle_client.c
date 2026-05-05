#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "kv.h"

void handle_client(int conn_fd, hash_table_t *table) {
    FILE *in = fdopen(dup(conn_fd), "r");
    if (!in) return;

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), in)) {
        char key[MAX_KEY_LEN + 1], val[MAX_VAL_LEN + 1];

        // 1. GET <key> -> Shared Read Lock (handled in kv_get)
        if (sscanf(line, "GET %256s", key) == 1) {
            char out_val[MAX_VAL_LEN + 1];
            int res = kv_get(table, key, out_val);

            if (res == 0) {
                dprintf(conn_fd, "VALUE %s\n", out_val);
            } else {
                dprintf(conn_fd, RESP_NOTFOUND);
            }
        }
        // 2. PUT <key> <value> [ttl] -> Exclusive Write Lock (handled in kv_put)
        else if (strncmp(line, "PUT", 3) == 0) {
            int ttl = 0;
            if (sscanf(line, "PUT %256s %256s %d", key, val, &ttl) >= 2) {
                kv_put(table, key, val, ttl);
                dprintf(conn_fd, RESP_OK);
            } else {
                dprintf(conn_fd, "ERROR Malformed PUT command\n");
            }
        }
        // 3. DEL <key> -> Exclusive Write Lock (handled in kv_del)
        else if (sscanf(line, "DEL %256s", key) == 1) {
            int res = kv_del(table, key);

            if (res == 0) {
                dprintf(conn_fd, RESP_OK);
            } else {
                dprintf(conn_fd, RESP_NOTFOUND);
            }
        }
        // 4. STATS -> Shared Read Lock (for consistency)
        else if (strncmp(line, "STATS", 5) == 0) {
            pthread_rwlock_rdlock(&table->rwlock);
            long k = table->keys, h = table->hits, m = table->misses, p = table->puts, d = table->dels;
            pthread_rwlock_unlock(&table->rwlock);

            dprintf(conn_fd, "STATS keys=%ld hits=%ld misses=%ld puts=%ld dels=%ld\n", k, h, m, p, d);
        }
        // 5. QUIT
        else if (strncmp(line, "QUIT", 4) == 0) {
            dprintf(conn_fd, RESP_BYE);
            break;
        }
        else {
            dprintf(conn_fd, "ERROR Malformed command\n");
        }
    }

    fclose(in);
}
