# -FlowQ-Distributed-Job-Queue
# FlowQ

A distributed job queue system built in C++ with priority scheduling, fault-tolerant retries, dead-letter queue, and a real-time monitoring dashboard.

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Redis](https://img.shields.io/badge/Redis-7.0-red.svg)](https://redis.io/)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-15-336791.svg)](https://www.postgresql.org/)
[![Docker](https://img.shields.io/badge/Docker-Compose-2496ED.svg)](https://www.docker.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---



FlowQ is a production-grade distributed job queue that lets client applications submit background jobs via a REST API. Jobs are scheduled by priority, processed by a horizontally scalable worker pool, and automatically retried on failure with exponential backoff. Failed jobs that exceed the retry limit are moved to a dead-letter queue for manual inspection.

**Performance:** 500+ jobs/sec throughput · P99 latency under 120ms · tested across 5 concurrent worker nodes

---

## Features

- **3-tier priority queues** — High, Medium, and Low priority lanes using Redis sorted sets
- **Exponential backoff retry** — Failed jobs retry after 2^n seconds (n = attempt number)
- **Dead-letter queue (DLQ)** — Jobs exceeding 5 retries are persisted to PostgreSQL for inspection
- **At-least-once delivery** — Redis SETNX-based job locking with TTL heartbeat prevents job loss on worker crash
- **Idempotency keys** — Duplicate job submissions return the original job ID safely
- **Rate limiting** — Token bucket algorithm per API client, enforced in Redis
- **Scheduled jobs** — Submit jobs for future execution using a Unix timestamp
- **Real-time dashboard** — React frontend receiving live job events over WebSocket
- **Horizontal scaling** — Stateless workers scale from 1 to N Docker containers with zero downtime
- **Graceful shutdown** — Workers finish the current job before exiting on SIGTERM

---

## Architecture

```
┌─────────────────┐     ┌─────────────────┐
│   Client apps   │     │   Cron jobs     │
└────────┬────────┘     └────────┬────────┘
         │                       │
         └──────────┬────────────┘
                    ▼
         ┌──────────────────────┐
         │      API Server      │
         │  REST · Rate limit   │
         └──────────┬───────────┘
                    │ ZADD (priority score)
                    ▼
         ┌──────────────────────────────────┐
         │       Redis Priority Queue       │
         │   High (1) · Med (2) · Low (3)   │
         │       sorted sets / ZPOPMIN      │
         └──────────┬───────────────────────┘
                    │ BRPOP / ZPOPMIN
                    ▼
         ┌──────────────────────────────────┐
         │           Worker Pool            │◄─── retry loop (exp. backoff)
         │  Stateless · Docker · scale N    │
         └──────┬───────────────────────────┘
                │                    │
                │ success            │ max retries exceeded
                ▼                    ▼
     ┌──────────────────┐   ┌────────────────────┐
     │   PostgreSQL     │   │  Dead-letter queue  │
     │  Job history     │   │  (PostgreSQL table) │
     └──────────────────┘   └────────────────────┘
```

### Component breakdown

| Component | Responsibility |
|---|---|
| **API Server** | Accepts job submissions, validates input, checks rate limits, pushes to Redis queue |
| **Redis Queue** | Stores jobs in sorted sets keyed by priority score; also manages idempotency locks and rate limit buckets |
| **Worker Pool** | N stateless workers competing for jobs via atomic ZPOPMIN; each worker runs in its own Docker container |
| **PostgreSQL** | Persistent storage for job history, results, audit trail, and DLQ entries |
| **WebSocket Server** | Broadcasts real-time job status events to the monitoring dashboard |
| **React Dashboard** | Displays live throughput, P99 latency, error rate, and DLQ count charts |

---

## Design decisions

### Why Redis sorted sets for priority queues?

Each job is stored in a Redis sorted set with its priority as the score — 1 for High, 2 for Medium, 3 for Low. Workers call `ZPOPMIN` which atomically retrieves and removes the lowest-score (highest-priority) job. This gives O(log n) insertion and O(1) dequeue, and the atomic operation ensures no two workers ever claim the same job.

Alternative considered: three separate FIFO lists (one per priority). Rejected because a single sorted set with a compound score (priority + timestamp) gives finer-grained ordering within the same priority tier.

### Why at-least-once delivery instead of exactly-once?

Exactly-once delivery in a distributed system requires two-phase commit or saga patterns, which adds significant complexity and latency. At-least-once delivery is simpler and sufficient when the downstream handlers are idempotent. FlowQ enforces idempotency via a per-job idempotency key — if the same job runs twice, the second execution is a no-op at the application level.

### How job locking prevents duplicate processing

When a worker dequeues a job, it immediately sets a Redis lock:

```
SET lock:job:<job_id> <worker_id> NX EX 30
```

`NX` means "only set if not exists" — this is atomic. If another worker somehow has the same job reference, the lock fails and that worker skips the job. A heartbeat goroutine extends the TTL every 10 seconds while the job is in progress. If the worker crashes, the TTL expires and the lock is released, allowing another worker to pick it up.

### Why exponential backoff?

Retrying immediately after a failure usually hits the same error (downstream service still down, database still overloaded). Exponential backoff — waiting 2, 4, 8, 16, 32 seconds between retries — gives the downstream system time to recover while not hammering it with repeated failures.

### Why C++?

Low-level control over threading (`std::thread`, `std::mutex`, `std::condition_variable`) means the worker pool can be tuned precisely — no runtime overhead from garbage collection or async runtimes. The hiredis C library gives direct Redis protocol access with measurable latency advantages. This translates directly to the benchmark numbers: 500 jobs/sec at P99 < 120ms.

---

## Getting started

### Prerequisites

- Docker and Docker Compose
- C++17 compiler (GCC 11+ or Clang 14+)
- CMake 3.16+
- Redis 7.0+
- PostgreSQL 15+

### Quick start with Docker (recommended)

```bash
# Clone the repo
git clone https://github.com/yourusername/flowq.git
cd flowq

# Start all services: API server, 3 workers, Redis, PostgreSQL
docker-compose up --build

# API is available at http://localhost:8080
# Dashboard is available at http://localhost:3000
```

### Build from source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y \
  libhiredis-dev \
  libpq-dev \
  uuid-dev \
  cmake \
  build-essential

# Clone and build
git clone https://github.com/yourusername/flowq.git
cd flowq
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run Redis and PostgreSQL separately, then:
./flowq_api    # Start API server on :8080
./flowq_worker # Start a worker (run multiple for scaling)
```

### Environment variables

| Variable | Default | Description |
|---|---|---|
| `REDIS_HOST` | `localhost` | Redis host |
| `REDIS_PORT` | `6379` | Redis port |
| `PG_HOST` | `localhost` | PostgreSQL host |
| `PG_PORT` | `5432` | PostgreSQL port |
| `PG_USER` | `flowq` | PostgreSQL user |
| `PG_PASSWORD` | — | PostgreSQL password |
| `PG_DATABASE` | `flowq` | PostgreSQL database name |
| `API_PORT` | `8080` | API server port |
| `WORKER_CONCURRENCY` | `4` | Threads per worker container |
| `MAX_RETRIES` | `5` | Max attempts before DLQ |
| `RATE_LIMIT_RPS` | `100` | Max job submissions per second per client |

---

## API reference

### Submit a job

```
POST /jobs
Content-Type: application/json
```

```json
{
  "type": "send_email",
  "payload": {
    "to": "user@example.com",
    "subject": "Welcome!"
  },
  "priority": "high",
  "idempotency_key": "email-user-123-welcome",
  "run_at": null
}
```

**Response:**
```json
HTTP 202 Accepted
{
  "job_id": "job_01J3KX8F...",
  "status": "pending"
}
```

| Field | Type | Required | Description |
|---|---|---|---|
| `type` | string | Yes | Job handler name |
| `payload` | object | Yes | Arbitrary JSON passed to the handler |
| `priority` | string | No | `high`, `med`, or `low` (default: `med`) |
| `idempotency_key` | string | No | Unique key to prevent duplicate jobs |
| `run_at` | Unix timestamp | No | Schedule job for future execution |

### Check job status

```
GET /jobs/:job_id
```

```json
{
  "job_id": "job_01J3KX8F...",
  "status": "done",
  "attempts": 1,
  "created_at": "2025-05-22T06:00:00Z",
  "completed_at": "2025-05-22T06:00:01Z",
  "result": { "message_id": "msg_abc123" }
}
```

Job status values: `pending` · `processing` · `done` · `failed` · `dead`

### List DLQ jobs

```
GET /dlq?limit=50&offset=0
```

### Retry a DLQ job manually

```
POST /dlq/:job_id/retry
```

### Health check

```
GET /health
→ { "status": "ok", "redis": "ok", "postgres": "ok" }
```

---

## Scaling workers

Workers are stateless — spin up as many as needed:

```bash
# Scale to 10 workers with Docker Compose
docker-compose up --scale worker=10

# Or run individual worker binaries on separate machines
REDIS_HOST=your-redis-host ./flowq_worker
```

Redis distributes jobs automatically across all workers via atomic `ZPOPMIN`. No coordination layer needed.

---

## Performance benchmarks

Tested on a local machine (Apple M2, 16GB RAM) with Docker Compose running 5 worker containers, each with 4 threads.

| Metric | Result |
|---|---|
| Throughput | 500 jobs/sec sustained |
| P50 latency | 18ms |
| P95 latency | 67ms |
| P99 latency | 112ms |
| Job loss on worker crash | 0 (at-least-once delivery) |
| Duplicate executions | 0 (idempotency keys) |

Run the load test yourself:

```bash
cd tests/load
./run_load_test.sh --jobs 10000 --workers 5 --concurrency 50
```

---

## Project structure

```
flowq/
├── src/
│   ├── api/
│   │   ├── server.cpp        # HTTP server setup and route registration
│   │   ├── handlers.cpp      # Job submission, status, DLQ endpoints
│   │   └── rate_limiter.cpp  # Token bucket rate limiter (Redis-backed)
│   ├── worker/
│   │   ├── worker.cpp        # Job polling loop and processing logic
│   │   ├── pool.cpp          # Thread pool management
│   │   └── heartbeat.cpp     # Lock TTL extension on active jobs
│   ├── queue/
│   │   ├── redis_queue.cpp   # Priority queue operations (ZADD, ZPOPMIN)
│   │   └── dlq.cpp           # Dead-letter queue logic
│   ├── db/
│   │   ├── postgres.cpp      # PostgreSQL connection pool
│   │   └── migrations/       # SQL schema migrations
│   └── common/
│       ├── job.hpp           # Job struct definition
│       └── config.hpp        # Environment config loader
├── dashboard/                # React monitoring dashboard
│   ├── src/
│   │   ├── App.jsx
│   │   └── components/
│   │       ├── ThroughputChart.jsx
│   │       ├── LatencyChart.jsx
│   │       └── DLQTable.jsx
│   └── package.json
├── tests/
│   ├── unit/                 # Unit tests per component
│   ├── integration/          # End-to-end job lifecycle tests
│   └── load/                 # Load testing scripts
├── docker-compose.yml
├── CMakeLists.txt
└── README.md
```

---

## Roadmap

- [ ] Redis Cluster support for horizontal queue scaling
- [ ] gRPC endpoint alongside REST
- [ ] Job chaining (trigger job B on completion of job A)
- [ ] Prometheus metrics endpoint + Grafana dashboard
- [ ] Web UI for DLQ management (retry / discard jobs manually)
- [ ] Circuit breaker per job type

---

## License

MIT — see [LICENSE](LICENSE) for details.
