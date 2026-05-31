// ThreadPool — fixed-size worker pool with a single task queue.
// Intentionally minimal: no priorities, no cancellation tokens, no task IDs.
// Heavy pipelines should give each stage its own pool to avoid head-of-line blocking.

#ifndef FORKLIFT_SHARED_THREAD_POOL_H_
#define FORKLIFT_SHARED_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace forklift::shared {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&)                 = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;

    template <typename F, typename... Args>
    auto submit(F&& fn, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    void shutdown();
    [[nodiscard]] std::size_t worker_count() const noexcept { return workers_.size(); }

private:
    void worker_loop();

    std::vector<std::thread>           workers_;
    std::queue<std::function<void()>>  tasks_;
    std::mutex                         mu_;
    std::condition_variable            cv_;
    std::atomic<bool>                  stop_{false};
};

template <typename F, typename... Args>
auto ThreadPool::submit(F&& fn, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>> {
    using R = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<R()>>(
        [f = std::forward<F>(fn),
         tup = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            return std::apply(std::move(f), std::move(tup));
        });
    auto fut = task->get_future();
    {
        std::lock_guard<std::mutex> lock(mu_);
        tasks_.emplace([task] { (*task)(); });
    }
    cv_.notify_one();
    return fut;
}

}  // namespace forklift::shared

#endif  // FORKLIFT_SHARED_THREAD_POOL_H_
