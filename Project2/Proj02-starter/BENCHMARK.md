# Benchmark Results

Configuration used for these runs: local `kvserver` on port `9099` with `8` worker threads and `1024` hash buckets. Workload for every run: `90%` reads, `10%` writes, and `10,000` operations per client.

| Concurrent Clients | Ops per Client | Read/Write Mix | Throughput (ops/sec) |
| --- | --- | --- | ---: |
| 1 | 10,000 | 90/10 | 89,408.79 |
| 4 | 10,000 | 90/10 | 203,367.77 |
| 16 | 10,000 | 90/10 | 345,174.28 |
| 64 | 10,000 | 90/10 | 335,521.61 |

Throughput increases clearly from 1 to 16 clients, but it does not scale linearly with the client count. The biggest gains come early, and the curve starts to flatten by 16 concurrent clients. At 64 clients, throughput is slightly lower than at 16 clients, which suggests the server has already reached its useful concurrency limit on this machine. The plateau is likely caused by contention in shared server structures, worker scheduling overhead, lock contention during writes, and the fact that the server only has 8 worker threads and 8 CPU cores to absorb the extra client pressure.
