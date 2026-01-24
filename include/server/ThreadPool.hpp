#pragma once

// ThreadPool.hpp - Worker thread pool for concurrent connections
// Bohot saare clients sambhalne lai threads!
// (Threads to handle many clients!)

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "utils/Logger.hpp"

namespace server {

/**
 * ThreadPool - Manage a pool of worker threads
 * 
 * Purpose: Concurrent client connections handle karo efficiently
 *          (Handle concurrent client connections efficiently)
 * 
 * How it works:
 * - Create N worker threads on startup
 * - Threads wait for tasks in a queue
 * - When client connects, add task to queue
 * - Worker thread picks up task and executes
 * - Thread goes back to waiting after task done
 * 
 * Benefits:
 * - Avoid creating/destroying threads (expensive!)
 * - Limit concurrent connections (resource management)
 * - Better performance than thread-per-connection
 */
class ThreadPool {
private:
    std::vector<std::thread> workers_;              // Worker threads
    std::queue<std::function<void()>> tasks_;       // Task queue
    
    mutable std::mutex queueMutex_;                 // Protect task queue (mutable for const methods)
    std::condition_variable condition_;             // Signal workers
    std::atomic<bool> stop_;                        // Shutdown flag
    
    size_t numThreads_;                             // Number of worker threads

public:
    /**
     * Constructor - Thread pool bana lo
     * (Constructor - Create thread pool)
     * 
     * @param numThreads: Number of worker threads (default: hardware_concurrency)
     */
    explicit ThreadPool(size_t numThreads = 0) : stop_(false) {
        // Agar threads specify nahi kitte, CPU cores de hisaab se decide karo
        // (If threads not specified, decide based on CPU cores)
        if (numThreads == 0) {
            numThreads_ = std::thread::hardware_concurrency();
            
            // Agar hardware_concurrency 0 return kare (rare), default 4 use karo
            // (If hardware_concurrency returns 0 (rare), use default 4)
            if (numThreads_ == 0) {
                numThreads_ = 4;
            }
        } else {
            numThreads_ = numThreads;
        }

        utils::Logger::info("ThreadPool bana rahe haan with " + std::to_string(numThreads_) + " workers!");
        utils::Logger::info("(ThreadPool starting with " + std::to_string(numThreads_) + " workers!)");

        // Worker threads create karo
        // (Create worker threads)
        for (size_t i = 0; i < numThreads_; ++i) {
            workers_.emplace_back([this, i] {
                workerThread(i);
            });
        }

        utils::Logger::info("ThreadPool tayar! " + std::to_string(numThreads_) + " kaamaey kaam te lage hain!");
        // (ThreadPool ready! X workers are on duty!)
    }

    /**
     * Destructor - Graceful shutdown
     * Saare threads nu band kar do te tasks complete hone da wait karo
     * (Stop all threads and wait for tasks to complete)
     */
    ~ThreadPool() {
        utils::Logger::info("ThreadPool band kar rahe haan...");
        // (Shutting down ThreadPool...)
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }

        // Saare threads nu signal bhejo
        // (Signal all threads)
        condition_.notify_all();

        // Saare threads khatam hone da wait karo
        // (Wait for all threads to finish)
        for (std::thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        utils::Logger::info("ThreadPool completely band ho gaya. Saare workers ruk gaye.");
        // (ThreadPool completely shut down. All workers stopped.)
    }

    // Copy/Move nahi kar sakde - ThreadPool unique hona chahida
    // (No copy/move - ThreadPool should be unique)
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * Task queue vich add karo
     * (Add task to queue)
     * 
     * @param task: Function to execute (void())
     * 
     * Example: pool.enqueue([](){ handleClient(socket); });
     */
    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            if (stop_) {
                utils::Logger::warn("ThreadPool band ho raha hai, naye tasks nahi le sakde!");
                // (ThreadPool is shutting down, can't accept new tasks!)
                return;
            }

            // Task queue vich add karo
            // (Add task to queue)
            tasks_.push(std::move(task));
        }

        // Kisi worker nu jagga do task execute karn lai
        // (Wake up a worker to execute task)
        condition_.notify_one();
    }

    /**
     * Active worker threads count
     * (Get count of active worker threads)
     */
    size_t getThreadCount() const {
        return numThreads_;
    }

    /**
     * Pending tasks count
     * (Get count of pending tasks)
     */
    size_t getPendingTaskCount() const {
        std::unique_lock<std::mutex> lock(queueMutex_);
        return tasks_.size();
    }

private:
    /**
     * Worker thread function
     * Continuously tasks execute karda raho jab tak shutdown nahi ho janda
     * (Continuously execute tasks until shutdown)
     */
    void workerThread(size_t workerId) {
        utils::Logger::debug("Worker thread #" + std::to_string(workerId) + " shuru ho gaya!");
        // (Worker thread started!)

        while (true) {
            std::function<void()> task;

            {
                // Queue se task nikalo
                // (Get task from queue)
                std::unique_lock<std::mutex> lock(queueMutex_);
                
                // Wait karo jab tak task nahi milega ya shutdown signal nahi ayega
                // (Wait until task arrives or shutdown signal)
                condition_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                });

                // Agar shutdown ho raha hai te queue khali hai, thread khatam karo
                // (If shutting down and queue empty, end thread)
                if (stop_ && tasks_.empty()) {
                    utils::Logger::debug("Worker #" + std::to_string(workerId) + " band ho raha hai.");
                    // (Worker shutting down.)
                    return;
                }

                // Agar task hai, queue se nikalo
                // (If task available, get from queue)
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
            }

            // Task execute karo (mutex ke bahar - concurrency lai!)
            // (Execute task outside mutex - for concurrency!)
            if (task) {
                try {
                    utils::Logger::debug("Worker #" + std::to_string(workerId) + " task execute kar raha hai...");
                    // (Worker executing task...)
                    
                    task();
                    
                    utils::Logger::debug("Worker #" + std::to_string(workerId) + " task complete!");
                    // (Worker completed task!)
                    
                } catch (const std::exception& e) {
                    utils::Logger::error("Worker #" + std::to_string(workerId) + 
                                       " task execution vich exception: " + std::string(e.what()));
                    // (Worker got exception during task execution)
                } catch (...) {
                    utils::Logger::error("Worker #" + std::to_string(workerId) + 
                                       " task execution vich unknown exception!");
                    // (Worker got unknown exception during task execution)
                }
            }
        }
    }
};

} // namespace server
