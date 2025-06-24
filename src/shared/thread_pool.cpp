#include "thread_pool.h"
#include <cstddef>

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] { workerThread(); });
    }
}

ThreadPool::~ThreadPool() {
    stop_ = true;
    cv_.notify_all();
    for (std::thread &worker : workers_) { worker.join(); }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.push(std::move(task));
    }
    ++tasks_to_complete; 
    cv_.notify_one();
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();  // Execute the task
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            --tasks_to_complete;
            if (tasks_to_complete == 0) cv_.notify_all();
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_.wait(lock, [this] { return tasks_to_complete == 0; });
}
