// ConcurrentQueue<T> — bounded MPMC queue with explicit drop-oldest semantics.
//
// Frame-dropping policy lives here (not in the producer) so every stage in the
// pipeline can opt into either back-pressure (BlockWhenFull) or drop-oldest
// (DropOldestWhenFull). Real-time vision pipelines typically pick the latter:
// retaining a stale frame is worse than dropping it.

#ifndef FORKLIFT_SHARED_CONCURRENT_QUEUE_H_
#define FORKLIFT_SHARED_CONCURRENT_QUEUE_H_

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace forklift::shared {

enum class OverflowPolicy {
    kBlockWhenFull,
    kDropOldestWhenFull,
};

template <typename T>
class ConcurrentQueue {
public:
    explicit ConcurrentQueue(std::size_t capacity,
                             OverflowPolicy policy = OverflowPolicy::kDropOldestWhenFull)
        : capacity_(capacity), policy_(policy) {}

    // Returns false if the queue was closed before the push could complete.
    bool push(T value) {
        std::unique_lock<std::mutex> lock(mu_);
        if (policy_ == OverflowPolicy::kBlockWhenFull) {
            not_full_.wait(lock, [&] { return closed_ || items_.size() < capacity_; });
            if (closed_) return false;
        } else {
            if (items_.size() >= capacity_) {
                items_.pop_front();
                ++dropped_;
            }
        }
        items_.push_back(std::move(value));
        not_empty_.notify_one();
        return true;
    }

    // Returns std::nullopt only when the queue is closed AND drained.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mu_);
        not_empty_.wait(lock, [&] { return closed_ || !items_.empty(); });
        if (items_.empty()) return std::nullopt;
        T v = std::move(items_.front());
        items_.pop_front();
        not_full_.notify_one();
        return v;
    }

    // Pop the most recent item, discarding any older backlog as dropped. Blocks
    // until an item is available or the queue is closed. Use this in realtime
    // consumers (e.g. inference) where a stale frame is worthless: it guarantees
    // the consumer always works on the freshest item and that a slow consumer
    // can never fall progressively further behind the live source.
    std::optional<T> pop_latest() {
        std::unique_lock<std::mutex> lock(mu_);
        not_empty_.wait(lock, [&] { return closed_ || !items_.empty(); });
        if (items_.empty()) return std::nullopt;
        while (items_.size() > 1) {
            items_.pop_front();
            ++dropped_;
        }
        T v = std::move(items_.front());
        items_.pop_front();
        not_full_.notify_one();
        return v;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    [[nodiscard]] std::size_t size()    const { std::lock_guard l(mu_); return items_.size(); }
    [[nodiscard]] std::size_t dropped() const { std::lock_guard l(mu_); return dropped_; }

private:
    mutable std::mutex      mu_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T>           items_;
    std::size_t             capacity_;
    OverflowPolicy          policy_;
    std::size_t             dropped_{0};
    bool                    closed_{false};
};

}  // namespace forklift::shared

#endif  // FORKLIFT_SHARED_CONCURRENT_QUEUE_H_
