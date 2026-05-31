// Entrypoint. Composes the system from configuration.
//
// Responsibilities (strictly composition root — no business logic):
//   1. Parse CLI flags
//   2. Load YAML config
//   3. Construct one FrameProcessingPipeline per camera, wiring in the
//      backend-specific InferenceEngine
//   4. Start the WebSocket publisher and all pipelines
//   5. Wait for SIGINT/SIGTERM, then orchestrate graceful shutdown

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "forklift/application/FrameProcessingPipeline.h"
#include "forklift/application/RiskDetectionService.h"
#include "forklift/application/SafetyZoneService.h"
#include "forklift/infrastructure/config/YamlConfigLoader.h"
#include "forklift/infrastructure/logging/Logger.h"
#include "forklift/infrastructure/transport/WebSocketAlertPublisher.h"
#include "forklift/infrastructure/video/RtspCameraSource.h"
#include "forklift/infrastructure/video/VideoFileSource.h"

#ifdef FSS_WITH_ONNXRUNTIME
#include "forklift/infrastructure/ai/OnnxInferenceEngine.h"
#endif

namespace {

std::atomic<bool> g_stop{false};

void handle_signal(int /*sig*/) { g_stop.store(true); }

std::string find_arg(int argc, char** argv, const std::string& key, const std::string& fallback) {
    for (int i = 1; i < argc - 1; ++i) {
        if (key == argv[i]) return argv[i + 1];
    }
    return fallback;
}

std::shared_ptr<forklift::application::InferenceEngine>
make_engine(forklift::infrastructure::config::InferenceBackend backend) {
    using namespace forklift;
    switch (backend) {
        case infrastructure::config::InferenceBackend::kOnnxRuntime:
#ifdef FSS_WITH_ONNXRUNTIME
            return std::make_shared<infrastructure::ai::OnnxInferenceEngine>();
#else
            LOG_ERROR("requested onnxruntime backend but binary was built without it");
            return nullptr;
#endif
        case infrastructure::config::InferenceBackend::kTensorRT:
            LOG_ERROR("tensorrt backend not yet wired into composition root");
            return nullptr;
    }
    return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace forklift;

    const std::string cfg_path = find_arg(argc, argv, "--config", "conf/system.yaml");
    LOG_INFO("forklift_safety starting config=" << cfg_path);

    auto cfg_res = infrastructure::config::load_from_yaml(cfg_path);
    if (!cfg_res) {
        LOG_ERROR("failed to load config: " << cfg_res.error().message);
        return 1;
    }
    const auto& cfg = cfg_res.value();

    if (cfg.cameras.empty()) {
        LOG_ERROR("no cameras configured");
        return 1;
    }

    // Singleton publisher fanning out alerts from every camera.
    auto publisher = std::make_shared<infrastructure::transport::WebSocketAlertPublisher>(
        cfg.websocket);
    if (auto r = publisher->start(); !r) {
        LOG_ERROR("publisher start failed: " << r.error().message);
        return 1;
    }

    // Shared domain services (immutable after construction).
    auto zone_service = std::make_shared<application::SafetyZoneService>(cfg.safety_zone);
    auto risk_service = std::make_shared<application::RiskDetectionService>(*zone_service);

    std::vector<std::unique_ptr<application::FrameProcessingPipeline>> pipelines;
    pipelines.reserve(cfg.cameras.size());

    for (const auto& cam : cfg.cameras) {
        std::shared_ptr<application::VideoSource> source;
        if (cam.source_type == infrastructure::config::CameraSourceType::kVideoFile) {
            source = std::make_shared<infrastructure::video::VideoFileSource>(cam.video_file);
        } else {
            source = std::make_shared<infrastructure::video::RtspCameraSource>(cam.rtsp);
        }
        auto engine = make_engine(cfg.backend);
        if (!engine) return 2;
        if (auto r = engine->initialize(cfg.inference); !r) {
            LOG_ERROR("engine init failed: " << r.error().message);
            return 2;
        }
        auto pipe = std::make_unique<application::FrameProcessingPipeline>(
            source, engine, risk_service, publisher, cam.pipeline);
        pipe->start();
        pipelines.push_back(std::move(pipe));
        LOG_INFO("pipeline started camera=" << source->camera_id());
    }

    std::signal(SIGINT,  handle_signal);
    std::signal(SIGTERM, handle_signal);

    while (!g_stop.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    LOG_INFO("shutdown requested — stopping pipelines");
    for (auto& p : pipelines) p->stop();
    publisher->stop();
    LOG_INFO("forklift_safety stopped cleanly");
    return 0;
}
