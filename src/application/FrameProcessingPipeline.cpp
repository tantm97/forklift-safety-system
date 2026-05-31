#include "forklift/application/FrameProcessingPipeline.h"

#include <chrono>
#include <thread>
#include <unordered_map>
#include <utility>

#include "forklift/infrastructure/logging/Logger.h"

namespace forklift::application {

FrameProcessingPipeline::FrameProcessingPipeline(
        std::shared_ptr<VideoSource>          source,
        std::shared_ptr<InferenceEngine>      engine,
        std::shared_ptr<RiskDetectionService> risk,
        std::shared_ptr<AlertPublisher>       publisher,
        PipelineConfig                        cfg)
    : source_(std::move(source)),
      engine_(std::move(engine)),
      risk_(std::move(risk)),
      publisher_(std::move(publisher)),
      cfg_(cfg),
      frames_(cfg.frame_queue_capacity, shared::OverflowPolicy::kDropOldestWhenFull) {}

FrameProcessingPipeline::~FrameProcessingPipeline() { stop(); }

void FrameProcessingPipeline::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    inference_pool_ = std::make_unique<shared::ThreadPool>(cfg_.inference_workers);
    capture_thread_ = std::thread(&FrameProcessingPipeline::capture_loop, this);
    for (std::size_t i = 0; i < cfg_.inference_workers; ++i) {
        inference_pool_->submit([this] { inference_loop(); });
    }
}

void FrameProcessingPipeline::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) return;
    frames_.close();
    if (capture_thread_.joinable()) capture_thread_.join();
    if (inference_pool_) inference_pool_->shutdown();
    if (source_) source_->close();
}

void FrameProcessingPipeline::capture_loop() {
    auto open_result = source_->open();
    if (!open_result) {
        LOG_ERROR("capture_loop: open() failed: " << open_result.error().message);
    }
    while (running_.load()) {
        if (!source_->is_open()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            (void)source_->open();
            continue;
        }
        auto frame_opt = source_->read_frame();
        if (!frame_opt) {
            LOG_WARN("capture_loop: read_frame returned empty — reconnecting");
            source_->close();
            continue;
        }
        frames_.push(std::move(*frame_opt));
    }
}

void FrameProcessingPipeline::inference_loop() {
    // Per-(camera, forklift_track) cooldown for duplicate suppression.
    std::unordered_map<int, domain::Alert::TimePoint> last_alert;

    while (running_.load()) {
        auto frame_opt = frames_.pop();
        if (!frame_opt) return;                   // queue closed + drained
        const auto& frame = *frame_opt;
        if (frame.empty()) continue;

        auto det_result = engine_->infer(frame);
        if (!det_result) {
            LOG_WARN("inference_loop: infer failed: " << det_result.error().message);
            continue;
        }

        const auto frame_ts = domain::Alert::Clock::now();
        auto alerts = risk_->evaluate(source_->camera_id(), det_result.value(), frame_ts);
        for (auto& a : alerts) {
            auto it = last_alert.find(a.forklift_track_id);
            if (it != last_alert.end() &&
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    frame_ts - it->second) < cfg_.alert_cooldown) {
                continue;  // within cooldown — drop
            }
            last_alert[a.forklift_track_id] = frame_ts;
            auto pub = publisher_->publish(a);
            if (!pub) {
                LOG_WARN("inference_loop: publish failed: " << pub.error().message);
            }
        }
    }
}

}  // namespace forklift::application
