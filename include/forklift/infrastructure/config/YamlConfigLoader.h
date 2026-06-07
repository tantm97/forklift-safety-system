// SystemConfig + YAML loader.
// Single source of truth for runtime configuration.

#ifndef FORKLIFT_INFRASTRUCTURE_CONFIG_YAML_CONFIG_LOADER_H_
#define FORKLIFT_INFRASTRUCTURE_CONFIG_YAML_CONFIG_LOADER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "forklift/application/FrameProcessingPipeline.h"
#include "forklift/application/InferenceEngine.h"
#include "forklift/application/SafetyZoneService.h"
#include "forklift/infrastructure/transport/MjpegStreamServer.h"
#include "forklift/infrastructure/transport/WebSocketAlertPublisher.h"
#include "forklift/infrastructure/video/RtspCameraSource.h"
#include "forklift/infrastructure/video/VideoFileSource.h"
#include "forklift/shared/Result.h"

namespace forklift::infrastructure::config {

enum class InferenceBackend { kOnnxRuntime, kTensorRT };
enum class CameraSourceType  { kRtsp, kVideoFile };

struct CameraEntry {
    CameraSourceType             source_type{CameraSourceType::kRtsp};
    video::RtspConfig            rtsp;
    video::VideoFileConfig       video_file;
    application::PipelineConfig  pipeline;
};

struct SystemConfig {
    InferenceBackend                    backend{InferenceBackend::kOnnxRuntime};
    application::InferenceConfig        inference;
    application::SafetyZoneConfig       safety_zone;
    transport::WebSocketServerConfig    websocket;
    transport::ViewerServerConfig       viewer;
    std::vector<CameraEntry>            cameras;
    int                                 inference_thread_pool_size{0};   // 0 = auto
    std::string                         log_level{"info"};
};

shared::Result<SystemConfig> load_from_yaml(const std::string& path);

}  // namespace forklift::infrastructure::config

#endif  // FORKLIFT_INFRASTRUCTURE_CONFIG_YAML_CONFIG_LOADER_H_
