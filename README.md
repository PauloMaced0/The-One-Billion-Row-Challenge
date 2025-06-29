# The One Billion Row Challenge 

## Instructions to Run the Programs

After running the `make` command, the executables will be created inside the `build/` folder.

### Weather Stations Instructions

- **cle-ws**: Accepts a file as input.

```sh
./build/cle-ws <input_file>
```

- **cle-ws-threads**: Accepts a file as input and the number of threads to be created.

```sh
./build/cle-ws-threads <input_file> <number_threads>
```

- **cle-ws-samples**: Accepts a file as input.

```sh
./build/cle-ws-samples <number_samples>
```

## Approach 

This program uses a different strategy by employing a **thread pool**, splitting the file into multiple chunks, and processing each chunk as a separate task in parallel.

## Performance Analysis

- Without Caching (cold cache):
    - The multithreaded version achieves a speedup of **46 x**, reducing runtime from `305 seconds` (single-threaded) to `6.6 seconds`. 

- With Caching (warm cache):
    - It is significantly faster thanks to the use of mmap, which allows the operating system to keep many pages already cached in memory, achieving a **112 x** speedup, executing in just `2.7 seconds`.

> [!NOTE]
> These tests were performed with a file of **500 million rows**, not **1 billion**, because there wasn’t enough physical memory to fully map and exploit parallelism at that scale.
With the smaller file, the entire dataset fits in memory, allowing all threads to stay busy and better demonstrate parallel performance.
