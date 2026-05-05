/*
 * kvserver.c -- Mini-KV server entry point
 *
 * Project 2, CprE 3080, Spring 2026
 *
 * Starter scaffolding: this file gives you a working TCP listener and an
 * argument parser. Everything else -- accept loop, protocol, hash table,
 * thread pool, RW locking, TTL sweeper -- is yours to write.
 *
 * Build: run `make` in this directory. See the provided Makefile.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "kv.h"

/* -------- Globals ------------------------------------------------------- */
// Handles Ctrl-C
volatile sig_atomic_t g_shutdown = 0;

static void sigint_handler(int sig) {
    (void)sig;
    g_shutdown = 1;
}

// Functions 
static void queue_enqueue(work_queue_t *q, int fd);
static int queue_dequeue(work_queue_t *q);
void *worker_main(void *arg);
void *sweeper_main(void *arg);
/* -------- Socket helpers ------------------------------------------------ */

/* Create a listening TCP socket bound to the given port. Returns fd or -1. */
static int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); // creates socket
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { // immediate server reset
        perror("setsockopt");
        close(fd);
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) { // assigns socket to port
        perror("bind");
        close(fd);
        return -1;
    }
    if (listen(fd, 64) < 0) { // Tells the OS to queue up incoming requests with buffer size 64
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

/* -------- Entry point --------------------------------------------------- */

static void usage(const char *prog) {
    fprintf(stderr,
        "usage: %s <port> <num_workers> <num_buckets> [sweeper_interval_ms]\n"
        "   port                TCP port to listen on (1-65535)\n"
        "   num_workers         number of worker threads (>=1)\n"
        "   num_buckets         hash-table bucket count (>=1)\n"
        "   sweeper_interval_ms default 500\n",
        prog);
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 5) {
        usage(argv[0]);
        return 1;
    }
    int port         = atoi(argv[1]);
    int num_workers  = atoi(argv[2]);
    int num_buckets  = atoi(argv[3]);
    int sweeper_ms   = (argc == 5) ? atoi(argv[4]) : 500;
    
    if (port <= 0 || port > 65535 || num_workers < 1 ||
        num_buckets < 1 || sweeper_ms <= 0) {
        usage(argv[0]);
        return 1;
    }

    /* Install Ctrl-C handler for clean shutdown. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Ignore SIGPIPE: writes to closed sockets should fail with EPIPE, not
     * kill the server. */
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = make_listen_socket(port);
    if (listen_fd < 0) return 1;

    /* Initialize the hash table */
    hash_table_t *table = kv_init(num_buckets, sweeper_ms);
    if (!table) {
        fprintf(stderr, "Failed to initialize hash table\n");
        return 1;
    }

    /* (Stage 2): Initialize work queue */
    work_queue_t queue;
    queue.capacity = num_workers * 2; 
    queue.fds = malloc(sizeof(int) * queue.capacity);
    queue.count = 0;
    queue.head = 0;
    queue.tail = 0;
    queue.table = table;
    pthread_mutex_init(&queue.lock, NULL);
    pthread_cond_init(&queue.not_empty, NULL);
    pthread_cond_init(&queue.not_full, NULL);

    /* (Stage 2): Spawn worker threads */
    pthread_t *workers = malloc(sizeof(pthread_t) * num_workers);
    for (int i = 0; i < num_workers; i++) {
        if (pthread_create(&workers[i], NULL, worker_main, &queue) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    /* (Stage 4): Spawn the sweeper thread. */
    pthread_t sweeper_tid;
    if (pthread_create(&sweeper_tid, NULL, sweeper_main, table) != 0) {
        perror("pthread_create (sweeper)");
        exit(1);
    }

    fprintf(stderr,
        "kvserver: listening on port %d "
        "(workers=%d, buckets=%d, sweeper=%dms)\n",
        port, num_workers, num_buckets, sweeper_ms);

    /* ================================================================
     * (Stage 2): Multi-threaded accept loop. */
    while (!g_shutdown) {
      struct sockaddr_in client_addr;
      socklen_t client_len = sizeof(client_addr);

      int conn = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);

      if (conn < 0) {
          if (errno == EINTR) continue;
          perror("accept");
          continue;
      }

      /* Enqueue the connection for a worker thread. */
      queue_enqueue(&queue, conn);
    }   

    /* Shutdown logic */
    fprintf(stderr, "kvserver: shutting down...\n");

    // Wake up all workers so they can exit
    pthread_mutex_lock(&queue.lock);
    pthread_cond_broadcast(&queue.not_empty);
    pthread_mutex_unlock(&queue.lock);

    // Join all worker threads
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }

    /* (Stage 4): Join the sweeper thread on shutdown. */
    pthread_join(sweeper_tid, NULL);

    /* Cleanup */
    free(workers);
    free(queue.fds);
    pthread_mutex_destroy(&queue.lock);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);

    /* (shutdown): free the hash table and any other resources. */
    kv_destroy(table);

    close(listen_fd);
    return 0;
}

static void queue_enqueue(work_queue_t *q, int fd) {
  pthread_mutex_lock(&q->lock);

  while (q->count == q->capacity) {
      pthread_cond_wait(&q->not_full, &q->lock);
  }

  q->fds[q->tail] = fd;
  q->tail = (q->tail + 1) % q->capacity;
  q->count++;

  pthread_cond_signal(&q->not_empty);

  pthread_mutex_unlock(&q->lock);
}

static int queue_dequeue(work_queue_t *q) {
    pthread_mutex_lock(&q->lock);

    while (q->count == 0 && !g_shutdown) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if (q->count == 0 && g_shutdown) {
        pthread_mutex_unlock(&q->lock);
        return -1; 
    }

    int fd = q->fds[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    pthread_cond_signal(&q->not_full);

    pthread_mutex_unlock(&q->lock);
    
    return fd;
}

void *worker_main(void *arg) {
    work_queue_t *q = (work_queue_t *)arg;
    
    // The worker loop runs until dequeue returns a sentinel (-1)
    while (1) {
        int conn = queue_dequeue(q);
        if (conn == -1) {
            break; 
        }
        handle_client(conn, q->table);

        close(conn);
    }

    return NULL;
}
