#include "server/ThreadPool.hpp"

// ThreadPool.cpp - Queue, condition-variable handshake and worker loop.

#include "utils/Logger.hpp"

namespace server {

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    if (numThreads == 0) {
        numThreads_ = std::thread::hardware_concurrency();
        // hardware_concurrency 0 return kare (rare), default 4 use karo
        if (numThreads_ == 0) {
            numThreads_ = 4;
        }
    } else {
        numThreads_ = numThreads;
    }

    utils::Logger::info("ThreadPool bana rahe haan with " + std::to_string(numThreads_) + " workers!");

    for (size_t i = 0; i < numThreads_; ++i) {
        workers_.emplace_back([this, i] {
            workerThread(i);
        });
    }

    utils::Logger::info("ThreadPool tayar! " + std::to_string(numThreads_) + " kaamaey kaam te lage hain!");
}

ThreadPool::~ThreadPool() {
    utils::Logger::info("ThreadPool band kar rahe haan...");

    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }

    condition_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    utils::Logger::info("ThreadPool completely band ho gaya. Saare workers ruk gaye.");
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);

        if (stop_) {
            utils::Logger::warn("ThreadPool band ho raha hai, naye tasks nahi le sakde!");
            return;
        }

        tasks_.push(std::move(task));
    }

    condition_.notify_one();
}

size_t ThreadPool::getPendingTaskCount() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

void ThreadPool::workerThread(size_t workerId) {
    utils::Logger::debug("Worker thread #" + std::to_string(workerId) + " shuru ho gaya!");

    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            condition_.wait(lock, [this] {
                return stop_ || !tasks_.empty();
            });

            // Shutdown + empty queue: this worker is done.
            if (stop_ && tasks_.empty()) {
                utils::Logger::debug("Worker #" + std::to_string(workerId) + " band ho raha hai.");
                return;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        // Task execute karo (mutex ke bahar - concurrency lai!)
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                utils::Logger::error("Worker #" + std::to_string(workerId) +
                                     " task vich exception: " + std::string(e.what()));
            } catch (...) {
                utils::Logger::error("Worker #" + std::to_string(workerId) +
                                     " task vich unknown exception!");
            }
        }
    }
}

} // namespace server
