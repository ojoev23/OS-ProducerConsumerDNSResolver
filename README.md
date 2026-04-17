# Multi-Threaded Producer-Consumer DNS Resolver (C)

This repository implements a multi-threaded DNS resolver in C using a producer-consumer design.
It reads hostnames from one or more input files, resolves each hostname to an IPv4 address, and writes
results to an output log file.

## What This Project Demonstrates

- C systems programming with POSIX APIs
- Multi-threading with pthreads
- Producer-consumer synchronization
- Shared bounded buffer protected with semaphores
- Thread-safe file logging with a mutex
- DNS resolution with getaddrinfo + inet_ntop

## High-Level Architecture

The main executable is `multi-lookup`, built from:

- `multi-lookup.c`: thread orchestration, DNS resolution, I/O, timing
- `array.c` / `array.h`: shared thread-safe bounded buffer implementation

Flow:

1. Main thread initializes shared buffer + synchronization primitives.
2. Main thread starts resolver threads (consumers).
3. Main thread starts requester threads (producers), one per input file.
4. Requester threads read hostnames and push them into the shared buffer.
5. Resolver threads pop hostnames, resolve IPv4 addresses, and append to output log.
6. After producers finish, main pushes sentinel `NULL` values to stop consumers.
7. Program joins threads, frees resources, and prints total runtime.

## Producer-Consumer Model

### Producers (Requester Threads)

- Function: `requester_thread(void* arg)`
- One producer thread is created per input file (`argc - 2` files)
- Opens assigned file and reads each hostname line-by-line
- Removes newline and enqueues hostname copy using `array_put(&s, strdup(hostname))`

### Consumers (Resolver Threads)

- Function: `resolver_thread(void* arg)`
- Fixed thread count: 8 resolver threads (`num_resolvers = 8`)
- Continuously dequeues hostnames with `array_get`
- Stops when dequeued value is `NULL` (sentinel)
- Resolves hostname to IPv4 and logs:
	- success: `hostname -> IPv4`
	- failure: `hostname -> NOT_RESOLVED`

## Shared Array + Semaphore Synchronization

The shared bounded buffer is in `array.h`:

- Capacity: `ARRAY_SIZE = 8`
- Storage: `char *arr[ARRAY_SIZE]`
- Index state: `int top`

Synchronization primitives:

- `sem_t mutex`: binary semaphore for mutual exclusion on buffer state
- `sem_t empty`: counts empty slots (initialized to `ARRAY_SIZE`)
- `sem_t full`: counts filled slots (initialized to `0`)

### Buffer Operations

- `array_put` (producer side):
	1. `sem_wait(empty)` blocks when full
	2. `sem_wait(mutex)` enters critical section
	3. Inserts item at `arr[top]`, then increments `top`
	4. `sem_post(mutex)` and `sem_post(full)`

- `array_get` (consumer side):
	1. `sem_wait(full)` blocks when empty
	2. `sem_wait(mutex)` enters critical section
	3. Decrements `top`, removes item from `arr[top]`
	4. `sem_post(mutex)` and `sem_post(empty)`

Note: this buffer behaves like a LIFO stack (last-in, first-out), not a FIFO queue.

## DNS Resolution Details

Resolution is performed in `resolve_ipv4_address` using:

- `getaddrinfo` with `AF_INET` and `SOCK_STREAM`
- Iteration through returned addresses
- `inet_ntop` to convert binary IPv4 address to string form

If no IPv4 address is found or lookup fails, the resolver logs `NOT_RESOLVED`.

## Thread-Safe Logging

- Shared output file pointer: `log_file_ptr`
- Lock: `pthread_mutex_t log_mutex`
- Resolver threads lock before each `fprintf` and unlock immediately after
- Prevents interleaved/corrupted output lines across threads

## Program Interface

Usage:

```bash
./multi-lookup <output_log_file> <input_file_1> [input_file_2 ...]
```

Example:

```bash
./multi-lookup results.txt hostnames/names1.txt hostnames/names2.txt hostnames/names3.txt
```

Input:

- Each input file contains one hostname per line.

Output:

- The output log file contains one line per processed hostname:
	- `example.com -> 93.184.216.34`
	- `bad-host.invalid -> NOT_RESOLVED`

Console output:

- Prints total elapsed runtime:
	- `./multi-lookup: total time is X.XXXXXX seconds`

## Build Instructions

The project uses a simple Makefile.

Build:

```bash
make
```

Clean:

```bash
make clean
```

Compiler and linker settings (from Makefile):

- `-Wall -Wextra -g -std=gnu99`
- Links with pthreads via `-lpthread`

## Concurrency and Resource Lifecycle

- Shared array semaphores initialized in `array_init` and destroyed in `array_free`
- Logging mutex initialized before thread creation and destroyed after joins
- Producers are joined before sending consumer stop signals
- Main sends exactly one `NULL` sentinel per resolver thread
- Hostname strings allocated by producers (`strdup`) are freed by consumers after logging

## Repository Contents

- `multi-lookup.c`: main logic, threads, DNS, timing
- `multi-lookup.h`: project header placeholder
- `array.c`: semaphore-based shared bounded buffer
- `array.h`: buffer API, struct, constants
- `hostnames/`: sample input hostname files
- `Makefile`: build/clean rules

## Notes

- Resolver thread count is currently fixed at 8.
- Buffer capacity is currently fixed at 8 via `ARRAY_SIZE`.
- Current shared-buffer ordering is LIFO due to stack-style `top` usage.
