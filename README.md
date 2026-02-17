# The One Billion Row Challenge 
The goal of this challenge is to write a program that reads temperature measurements from a text file and computes the **minimum**, **mean**, and **maximum** temperature for each weather station.

There is just one caveat:

> The input file contains **1,000,000,000 rows** — over **10 GB** of data. 

## Instructions to Run the Programs

After running the `make` command, the executables will be created inside the `build/` folder.

### Weather Stations Programs

#### Generate Sample files

`cle-ws-samples`

Generates a sample dataset.

```sh
./build/cle-ws-samples <number_samples>
```

**Arguments**:

- `<number_samples>` - Number of rows to generate

#### Execute Programs 

**Single-Threaded Version —** `cle-ws`

Processes the input file using a single thread.

```sh
./build/cle-ws <input_file>
```

**Arguments**:

- `<input_file>` – Path to the dataset file.

---

**Multi-Threaded Version —** `cle-ws-threads`

Processes the input file using a configurable number of threads.

```sh
./build/cle-ws-threads <input_file> <number_threads>
```

**Arguments**:

- `<input_file>` – Path to the dataset file

- `<number_of_threads>` – Number of worker threads to create

## Approach 

This implementation uses a thread pool to efficiently process large files.

### Strategy:

1. The file is split into multiple **chunks**.

2. Each chunk becomes a **task**.

3. Tasks are placed into a **task queue**.

4. Worker threads from the **thread pool** continuously pull tasks from the queue.

5. Each worker processes its assigned chunk in parallel.

This approach:

- Maximizes CPU utilization

- Reduces synchronization overhead (mostly computing, rarely blocking)

- Scales with the number of available cores

### Thread Pool Architecture

[Thread Pool](./thread_pool.png)

## Performance Analysis

### Single-Threaded Execution

In the single-threaded scenario, the program takes **548.9 seconds**.

### Multi-Threaded Execution

Performance improves significantly when using the thread pool.

#### Cold Cache (No OS Caching)

When the file is not cached in memory (first run):

- Runtime: **22.6 seconds**

- Speedup: **~24× faster**

This improvement comes purely from parallel processing, multiple threads working on different chunks simultaneously.

---

#### Warm Cache (With OS Page Cache)

When the file is already cached in memory (subsequent runs):

- Runtime: **7.8 seconds**

- Speedup: **~70× faster**

This additional gain is largely due to:

- Use of `mmap`

- The operating system keeping file pages in memory (page cache)

- Reduced disk I/O overhead

Because the data is already in RAM, the program becomes CPU-bound rather than I/O-bound.

### Benchmark Hardware

Benchmarks were executed on:

- **CPU**: AMD EPYC 8434P (8 cores, 8 threads)

- **Architecture**: x86_64

- **L3 Cache**: 128 MB

- **Virtualized Environment**: KVM (AMD-V)
