#include "forklift/infrastructure/config/YamlConfigLoader.h"

#include <fstream>

#include "forklift/infrastructure/logging/Logger.h"

#ifdef FSS_HAS_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace forklift::infrastructure::config {

namespace {

#ifdef FSS_HAS_YAML_CPP

template <typename T>
T get_or(const YAML::Node& n, const std::string& key, T fallback) {
    if (n[key]) return n[key].as<T>();
    return fallback;
}

InferenceBackend parse_backend(const std::string& s) {
    if (s == "tensorrt" || s == "trt") return InferenceBackend::kTensorRT;
    return InferenceBackend::kOnnxRuntime;
}

application::SafetyZoneConfig::Mode parse_zone_mode(const std::string& s) {
    if (s == "absolute") return application::SafetyZoneConfig::Mode::kAbsolutePixels;
    return application::SafetyZoneConfig::Mode::kRelativeToBox;
}

#endif

}  // namespace

shared::Result<SystemConfig> load_from_yaml(const std::string& path) {
#ifndef FSS_HAS_YAML_CPP
    (void)path;
    return shared::Error{100, "yaml-cpp was not available at build time"};
#else
    SystemConfig out;
    try {
        YAML::Node root = YAML::LoadFile(path);

        out.backend   = parse_backend(get_or<std::string>(root, "backend", "onnxruntime"));
        out.log_level = get_or<std::string>(root, "log_level", "info");
        out.inference_thread_pool_size =
            get_or<int>(root, "inference_thread_pool_size", 0);

        if (auto inf = root["inference"]) {
            out.inference.model_path     = get_or<std::string>(inf, "model_path", "");
            out.inference.input_width    = get_or<int>(inf, "input_width", 640);
            out.inference.input_height   = get_or<int>(inf, "input_height", 640);
            out.inference.conf_threshold = get_or<float>(inf, "conf_threshold", 0.35F);
            out.inference.nms_threshold  = get_or<float>(inf, "nms_threshold", 0.45F);
            out.inference.max_detections = get_or<int>(inf, "max_detections", 300);
        }

        if (auto sz = root["safety_zone"]) {
            out.safety_zone.mode = parse_zone_mode(
                get_or<std::string>(sz, "mode", "relative"));
            out.safety_zone.pad_x_px  = get_or<float>(sz, "pad_x_px", 120.0F);
            out.safety_zone.pad_y_px  = get_or<float>(sz, "pad_y_px", 120.0F);
            out.safety_zone.pad_ratio = get_or<float>(sz, "pad_ratio", 0.5F);
        }

        if (auto ws = root["websocket"]) {
            out.websocket.host = get_or<std::string>(ws, "host", "0.0.0.0");
            out.websocket.port = static_cast<std::uint16_t>(get_or<int>(ws, "port", 8765));
            out.websocket.path = get_or<std::string>(ws, "path", "/ws/alerts");
        }

        if (auto cams = root["cameras"]) {
            for (const auto& c : cams) {
                CameraEntry e;
                const std::string id  = get_or<std::string>(c, "id", "cam");
                const std::string src = get_or<std::string>(c, "source_type", "rtsp");

                if (src == "video_file") {
                    e.source_type             = CameraSourceType::kVideoFile;
                    e.video_file.camera_id    = id;
                    e.video_file.file_path    = get_or<std::string>(c, "video_path", "");
                    e.video_file.loop         = get_or<bool>(c, "loop", true);
                } else {
                    e.source_type    = CameraSourceType::kRtsp;
                    e.rtsp.camera_id = id;
                    e.rtsp.rtsp_url  = get_or<std::string>(c, "rtsp_url", "");
                }

                e.pipeline.frame_queue_capacity =
                    static_cast<std::size_t>(get_or<int>(c, "queue_capacity", 4));
                e.pipeline.inference_workers =
                    static_cast<std::size_t>(get_or<int>(c, "workers", 1));
                e.pipeline.alert_cooldown =
                    std::chrono::milliseconds(get_or<int>(c, "alert_cooldown_ms", 2000));
                e.pipeline.enable_viewer = get_or<bool>(c, "enable_viewer", false);
                out.cameras.push_back(std::move(e));
            }
        }
        LOG_INFO("config loaded path=" << path
                 << " cameras=" << out.cameras.size());
        return out;
    } catch (const std::exception& e) {
        return shared::Error{101, std::string{"yaml parse failed: "} + e.what()};
    }
#endif
}

}  // namespace forklift::infrastructure::config
