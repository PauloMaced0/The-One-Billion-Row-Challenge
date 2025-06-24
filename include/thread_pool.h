#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool (size_t numThreads);
    ~ThreadPool ();

    void enqueue(std::function<void()> task);

    // Wait until all tasks in the queue are processed
    void wait();

private:
    // Vector to store worker threads
    std::vector<std::thread> workers_;

    // Queue of tasks
    std::queue<std::function<void()>> tasks_;
    
    // Mutex to synchronize access to shared data
    std::mutex queue_mutex_;

    // Condition variable to signal changes in the state of the tasks queue
    std::condition_variable cv_;

    // Flag to indicate whether the thread pool should stop or not
    std::atomic<bool> stop_{false};

    std::atomic<uint> tasks_to_complete{0};

    void workerThread();
};

#endif // !THREAD_POOL_H
