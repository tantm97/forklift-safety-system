// FrameProcessingPipeline — the per-camera orchestrator.
//
//   capture-thread   →  frame_queue  →  worker pool  →  alert publisher
//
// Stages:
//   1. Capture loop reads frames from VideoSource and pushes them into a bounded
//      ConcurrentQueue with DropOldestWhenFull policy (keeps latency low).
//   2. A pool of inference workers pops frames, runs InferenceEngine,
//      hands the detections to RiskDetectionService, and forwards any Alert to
//      AlertPublisher.
//   3. A simple per-(camera, forklift_track) cooldown suppresses duplicate
//      alerts within a configurable window.

#ifndef FORKLIFT_APPLICATION_FRAME_PROCESSING_PIPELINE_H_
#define FORKLIFT_APPLICATION_FRAME_PROCESSING_PIPELINE_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "forklift/application/AlertPublisher.h"
#include "forklift/application/InferenceEngine.h"
#include "forklift/application/RiskDetectionService.h"
#include "forklift/application/VideoSource.h"
#include "forklift/domain/Frame.h"
#include "forklift/shared/ConcurrentQueue.h"
#include "forklift/shared/ThreadPool.h"

namespace forklift::application {

struct PipelineConfig {
    std::size_t              frame_queue_capacity{4};
    std::size_t              inference_workers{1};   // per camera
    std::chrono::milliseconds alert_cooldown{2000};
    bool                     enable_viewer{false};   // draw overlays + push viewer stream
};

class FrameProcessingPipeline {
public:
    FrameProcessingPipeline(std::shared_ptr<VideoSource>          source,
                            std::shared_ptr<InferenceEngine>      engine,
                            std::shared_ptr<RiskDetectionService> risk,
                            std::shared_ptr<AlertPublisher>       publisher,
                            PipelineConfig                        cfg);
    ~FrameProcessingPipeline();

    FrameProcessingPipeline(const FrameProcessingPipeline&)            = delete;
    FrameProcessingPipeline& operator=(const FrameProcessingPipeline&) = delete;

    void start();
    void stop();
    [[nodiscard]] bool running() const noexcept { return running_.load(); }

private:
    void capture_loop();
    void inference_loop();

    std::shared_ptr<VideoSource>          source_;
    std::shared_ptr<InferenceEngine>      engine_;
    std::shared_ptr<RiskDetectionService> risk_;
    std::shared_ptr<AlertPublisher>       publisher_;
    PipelineConfig                        cfg_;

    shared::ConcurrentQueue<domain::Frame> frames_;
    std::thread                            capture_thread_;
    std::unique_ptr<shared::ThreadPool>    inference_pool_;
    std::atomic<bool>                      running_{false};
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_FRAME_PROCESSING_PIPELINE_H_
