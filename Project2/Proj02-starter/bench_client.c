/*
 * bench_client.c -- Mini-KV concurrent benchmark client
 *
 * Project 2, CprE 3080, Spring 2026
 *
 * YOU write this file. The scaffolding here is intentionally minimal:
 * an argument parser and nothing else. Your job is to fill in the rest.
 *
 * Usage:
 *   ./bench_client <host> <port> <num_clients> <ops_per_client> <read_pct>
 *
 * Requirements (from the spec):
 *   - Spawn <num_clients> threads.
 *   - Each thread opens its own TCP connection to <host>:<port>.
 *   - Each thread issues <ops_per_client> operations.
 *   - <read_pct> percent of ops are GETs; the rest are PUTs.
 *   - Keys drawn from a small pool (~1000 keys) so GETs actually hit.
 *   - Report total wall-clock time and overall throughput (ops/sec).
 *
 * Hints:
 *   - Use clock_gettime(CLOCK_MONOTONIC, ...) to measure elapsed time.
 *   - Each thread needs its own rand_r() seed to avoid serialization on
 *     the global rand() lock.
 *   - Read the server's reply line-by-line; don't assume one TCP packet
 *     per command.
 */

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define KEY_POOL_SIZE 1000
#define KEY_BUF_SIZE  32
#define VAL_BUF_SIZE  64

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int total_threads;
    int ready_threads;
    int start;
} start_gate_t;

typedef struct {
    const char *host;
    const char *port_str;
    int ops_per_client;
    int read_pct;
    int thread_index;
    start_gate_t *gate;
    long completed_ops;
    long failed_ops;
} worker_args_t;

static void start_gate_init(start_gate_t *gate, int total_threads) {
    pthread_mutex_init(&gate->lock, NULL);
    pthread_cond_init(&gate->cond, NULL);
    gate->total_threads = total_threads;
    gate->ready_threads = 0;
    gate->start = 0;
}

static void start_gate_destroy(start_gate_t *gate) {
    pthread_mutex_destroy(&gate->lock);
    pthread_cond_destroy(&gate->cond);
}

static void start_gate_worker_ready(start_gate_t *gate) {
    pthread_mutex_lock(&gate->lock);
    gate->ready_threads++;
    pthread_cond_broadcast(&gate->cond);
    while (!gate->start) {
        pthread_cond_wait(&gate->cond, &gate->lock);
    }
    pthread_mutex_unlock(&gate->lock);
}

static void start_gate_wait_until_ready(start_gate_t *gate) {
    pthread_mutex_lock(&gate->lock);
    while (gate->ready_threads < gate->total_threads) {
        pthread_cond_wait(&gate->cond, &gate->lock);
    }
    pthread_mutex_unlock(&gate->lock);
}

static void start_gate_release(start_gate_t *gate) {
    pthread_mutex_lock(&gate->lock);
    gate->start = 1;
    pthread_cond_broadcast(&gate->cond);
    pthread_mutex_unlock(&gate->lock);
}

static double elapsed_seconds(const struct timespec *start,
                              const struct timespec *end) {
    time_t sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    return (double)sec + (double)nsec / 1000000000.0;
}

static int open_connection(const char *host, const char *port_str) {
    struct addrinfo hints;
    struct addrinfo *results = NULL;
    struct addrinfo *rp;
    int fd = -1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, port_str, &hints, &results);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo(%s:%s): %s\n",
                host, port_str, gai_strerror(rc));
        return -1;
    }

    for (rp = results; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(results);
    return fd;
}

static FILE *open_connection_stream(const char *host, const char *port_str) {
    int fd = open_connection(host, port_str);
    FILE *stream;

    if (fd < 0) {
        return NULL;
    }

    stream = fdopen(fd, "r+");
    if (!stream) {
        perror("fdopen");
        close(fd);
        return NULL;
    }

    setvbuf(stream, NULL, _IOLBF, 0);
    return stream;
}

static int expect_response(FILE *stream, const char *expected_prefix) {
    char line[512];

    if (!fgets(line, sizeof(line), stream)) {
        return -1;
    }

    if (strncmp(line, expected_prefix, strlen(expected_prefix)) != 0) {
        return -1;
    }
    return 0;
}

static int read_get_response(FILE *stream) {
    char line[512];

    if (!fgets(line, sizeof(line), stream)) {
        return -1;
    }

    if (strncmp(line, "VALUE ", 6) == 0 || strcmp(line, "NOT_FOUND\n") == 0) {
        return 0;
    }
    return -1;
}

static void make_key(int index, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "key%04d", index);
}

static void make_value(int thread_index, int op_index, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "value_t%d_o%d", thread_index, op_index);
}

static int preload_keys(const char *host, const char *port_str) {
    FILE *stream = open_connection_stream(host, port_str);
    char key[KEY_BUF_SIZE];
    char value[VAL_BUF_SIZE];

    if (!stream) {
        return -1;
    }

    for (int i = 0; i < KEY_POOL_SIZE; i++) {
        make_key(i, key, sizeof(key));
        snprintf(value, sizeof(value), "seed_%d", i);

        if (fprintf(stream, "PUT %s %s\n", key, value) < 0) {
            fclose(stream);
            return -1;
        }
        if (fflush(stream) == EOF || expect_response(stream, "OK") != 0) {
            fclose(stream);
            return -1;
        }
    }

    if (fprintf(stream, "QUIT\n") >= 0) {
        fflush(stream);
        (void)expect_response(stream, "BYE");
    }
    fclose(stream);
    return 0;
}

static void *worker_main(void *arg) {
    worker_args_t *worker = arg;
    FILE *stream = open_connection_stream(worker->host, worker->port_str);
    char key[KEY_BUF_SIZE];
    char value[VAL_BUF_SIZE];
    unsigned int seed;

    if (!stream) {
        worker->failed_ops = worker->ops_per_client;
        return NULL;
    }

    seed = (unsigned int)time(NULL) ^
           (unsigned int)(uintptr_t)pthread_self() ^
           (unsigned int)(worker->thread_index * 2654435761u);

    start_gate_worker_ready(worker->gate);

    for (int i = 0; i < worker->ops_per_client; i++) {
        int key_index = (int)(rand_r(&seed) % KEY_POOL_SIZE);
        int do_get = (int)(rand_r(&seed) % 100) < worker->read_pct;
        make_key(key_index, key, sizeof(key));

        if (do_get) {
            if (fprintf(stream, "GET %s\n", key) < 0 || fflush(stream) == EOF) {
                worker->failed_ops++;
                continue;
            }
            if (read_get_response(stream) == 0) {
                worker->completed_ops++;
            } else {
                worker->failed_ops++;
            }
        } else {
            make_value(worker->thread_index, i, value, sizeof(value));
            if (fprintf(stream, "PUT %s %s\n", key, value) < 0 ||
                fflush(stream) == EOF) {
                worker->failed_ops++;
                continue;
            }
            if (expect_response(stream, "OK") == 0) {
                worker->completed_ops++;
            } else {
                worker->failed_ops++;
            }
        }
    }

    if (fprintf(stream, "QUIT\n") >= 0) {
        fflush(stream);
        (void)expect_response(stream, "BYE");
    }
    fclose(stream);
    return NULL;
}

static void usage(const char *prog) {
    fprintf(stderr,
        "usage: %s <host> <port> <num_clients> <ops_per_client> <read_pct>\n",
        prog);
}

int main(int argc, char **argv) {
    if (argc != 6) {
        usage(argv[0]);
        return 1;
    }

    const char *host      = argv[1];
    const char *port_str  = argv[2];
    int port              = atoi(argv[2]);
    int num_clients       = atoi(argv[3]);
    int ops_per_client    = atoi(argv[4]);
    int read_pct          = atoi(argv[5]);
    pthread_t *threads    = NULL;
    worker_args_t *args   = NULL;
    start_gate_t gate;
    struct timespec start_time;
    struct timespec end_time;
    long total_completed = 0;
    long total_failed = 0;
    double total_seconds;
    double throughput;

    if (port <= 0 || port > 65535 || num_clients < 1 || ops_per_client < 1 ||
        read_pct < 0 || read_pct > 100) {
        usage(argv[0]);
        return 1;
    }

    if (preload_keys(host, port_str) != 0) {
        fprintf(stderr, "bench_client: failed to preload key set\n");
        return 1;
    }

    threads = calloc((size_t)num_clients, sizeof(*threads));
    args = calloc((size_t)num_clients, sizeof(*args));
    if (!threads || !args) {
        perror("calloc");
        free(threads);
        free(args);
        return 1;
    }

    start_gate_init(&gate, num_clients);

    for (int i = 0; i < num_clients; i++) {
        args[i].host = host;
        args[i].port_str = port_str;
        args[i].ops_per_client = ops_per_client;
        args[i].read_pct = read_pct;
        args[i].thread_index = i;
        args[i].gate = &gate;

        if (pthread_create(&threads[i], NULL, worker_main, &args[i]) != 0) {
            perror("pthread_create");
            gate.total_threads = i;
            start_gate_release(&gate);
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            start_gate_destroy(&gate);
            free(threads);
            free(args);
            return 1;
        }
    }

    start_gate_wait_until_ready(&gate);
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    start_gate_release(&gate);

    for (int i = 0; i < num_clients; i++) {
        pthread_join(threads[i], NULL);
        total_completed += args[i].completed_ops;
        total_failed += args[i].failed_ops;
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    total_seconds = elapsed_seconds(&start_time, &end_time);
    throughput = (total_seconds > 0.0) ? ((double)total_completed / total_seconds) : 0.0;

    printf("clients=%d ops_per_client=%d read_pct=%d\n",
           num_clients, ops_per_client, read_pct);
    printf("completed_ops=%ld failed_ops=%ld total_ops=%ld\n",
           total_completed, total_failed, total_completed + total_failed);
    printf("wall_clock_seconds=%.6f\n", total_seconds);
    printf("throughput_ops_per_sec=%.2f\n", throughput);

    start_gate_destroy(&gate);
    free(threads);
    free(args);
    return 0;
}
