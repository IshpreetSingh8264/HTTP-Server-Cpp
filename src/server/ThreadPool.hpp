#pragma once

// ThreadPool.hpp - Worker thread pool for concurrent connections
// Bohot saare clients sambhalne lai threads!
// (Threads to handle many clients!)

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace server {

/**
 * ThreadPool - Fixed pool of worker threads fed by a task queue
 *
 * Purpose: Concurrent client connections handle karo efficiently
 *          (Handle concurrent client connections efficiently)
 *
 * Created once at startup; each accepted socket becomes one task. Tasks run
 * outside the queue mutex, so workers are genuinely parallel.
 */
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex queueMutex_;  // Protect task queue (mutable for const methods)
    std::condition_variable condition_;
    std::atomic<bool> stop_;

    size_t numThreads_;

    void workerThread(size_t workerId);

public:
    /// @param numThreads 0 means std::thread::hardware_concurrency(), minimum 4.
    explicit ThreadPool(size_t numThreads = 0);

    /// Graceful shutdown: drain the queue, then join every worker.
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// Queue a task and wake one worker. Ignored once shutdown has begun.
    void enqueue(std::function<void()> task);

    size_t getThreadCount() const { return numThreads_; }
    size_t getPendingTaskCount() const;
};

} // namespace server
