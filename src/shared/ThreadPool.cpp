#include "forklift/shared/ThreadPool.h"

namespace forklift::shared {

ThreadPool::ThreadPool(std::size_t worker_count) {
    workers_.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::shutdown() {
    bool expected = false;
    if (!stop_.compare_exchange_strong(expected, true)) return;
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait(lock, [this] { return stop_.load() || !tasks_.empty(); });
            if (stop_.load() && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

}  // namespace forklift::shared
