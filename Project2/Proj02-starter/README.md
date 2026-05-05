# Mini-KV Server

## Build

Build the server:

```sh
make
```

Build the benchmark client:

```sh
make bench
```

Build both:

```sh
make all bench
```

Clean build artifacts:

```sh
make clean
```

## Run

Start the server:

```sh
./kvserver <port> <num_workers> <num_buckets> [sweeper_interval_ms]
```

Example:

```sh
./kvserver 9099 8 1024 500
```

Run the benchmark client:

```sh
./bench_client <host> <port> <num_clients> <ops_per_client> <read_pct>
```

Example:

```sh
./bench_client 127.0.0.1 9099 64 10000 90
```

The benchmark client prints total completed operations, failed operations, wall-clock time, and throughput in operations per second. A sample benchmark summary for the 1/4/16/64-client sweep is in [BENCHMARK.md](/home/zephaniah/Documents/SchoolProjects/CPRE_3080_Labs/Project2/Proj02-starter/BENCHMARK.md).

## Locking Design

This implementation uses a single table-wide read/write lock, `pthread_rwlock_t rwlock`, around the hash table. The main reason is correctness and simplicity: every `GET` can run concurrently with other `GET`s, while `PUT`, `DEL`, and expiration cleanup get exclusive access so pointer updates and frees are safe. For a class project, this is much easier to reason about than a finer-grained scheme, and it keeps the code short enough to verify.

The tradeoff is that all writes contend on one lock even when they target different buckets. Bucket-level locking would allow unrelated buckets to proceed independently and would usually improve write-heavy or mixed workloads, but it adds more bookkeeping and makes multi-bucket or whole-table operations harder to implement correctly. Per-entry locking is even more concurrent in principle, but it is also the most complex: insertion, deletion, traversal, and expiration become much easier to get wrong because lock ordering and memory reclamation matter a lot more.

The benchmark results reflect that tradeoff. With a `90%` read and `10%` write workload, throughput improved as concurrency increased, but it did not scale linearly: measured throughput rose from `89,408.79` ops/sec at 1 client to `345,174.28` ops/sec at 16 clients, then dipped slightly to `335,521.61` ops/sec at 64 clients. That shape is consistent with a design that handles read sharing well but eventually runs into shared-lock contention, worker scheduling overhead, and a fixed number of CPU cores.

## Worker Scaling

To study the effect of worker count, I ran the same benchmark workload at `64` concurrent clients, `10,000` operations per client, and `90/10` read/write mix while varying only the server worker threads. The server used `1024` buckets in all runs.

| Workers | Throughput (ops/sec) |
| --- | ---: |
| 2 | 148,481.81 |
| 4 | 195,317.36 |
| 8 | 301,806.67 |
| 16 | 297,943.45 |

Performance improved from 2 to 8 workers, then essentially stopped helping at 16 workers. On this machine, `nproc` reported 8 CPU cores, so that plateau makes sense: once there are enough workers to keep the cores busy, extra threads mostly add context-switching and lock contention instead of useful parallelism. The single table-wide RW lock also becomes a larger bottleneck as more workers compete to enter write sections or wait behind the sweeper.

## Sweeper Behavior

The sweeper thread wakes up every `sweeper_interval_ms`, scans the table for expired entries, and removes them. It interacts with worker threads through the same table-wide RW lock used by `GET`, `PUT`, and `DEL`, so any time the sweeper holds the write lock, workers cannot make progress on table operations.

If the write lock were held for the entire sweep pass over a large table, every reader and writer would block until that pass completed. That would create latency spikes, lower throughput, and increase the chance that many worker threads wake up only to pile up behind the lock. In the current code, that full-pass write lock is exactly what happens: the sweeper acquires the write lock once, walks every bucket, frees expired entries, and then releases it at the end of the scan. A more scalable version would break the work into smaller critical sections, such as locking one bucket at a time, so cleanup does not pause the whole server for the duration of an entire sweep.
