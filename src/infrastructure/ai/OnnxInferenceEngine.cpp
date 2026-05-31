// ONNX Runtime engine implementation — only compiled when FSS_WITH_ONNXRUNTIME=1.
#include "forklift/infrastructure/ai/OnnxInferenceEngine.h"

#ifdef FSS_WITH_ONNXRUNTIME

#include <onnxruntime_cxx_api.h>

#include <opencv2/imgproc.hpp>

#include "forklift/infrastructure/ai/YoloV8Postprocessor.h"
#include "forklift/infrastructure/logging/Logger.h"

namespace forklift::infrastructure::ai {

struct OnnxInferenceEngine::Impl {
    application::InferenceConfig         cfg{};
    std::unique_ptr<Ort::Env>            env;
    std::unique_ptr<Ort::Session>        session;
    std::vector<std::string>             input_names_storage;
    std::vector<std::string>             output_names_storage;
    std::vector<const char*>             input_names;
    std::vector<const char*>             output_names;
    PostprocessParams                    pp;
};

OnnxInferenceEngine::OnnxInferenceEngine() : impl_(std::make_unique<Impl>()) {}
OnnxInferenceEngine::~OnnxInferenceEngine() = default;

shared::Result<void> OnnxInferenceEngine::initialize(const application::InferenceConfig& cfg) {
    impl_->cfg = cfg;
    try {
        impl_->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "forklift");
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(1);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        impl_->session = std::make_unique<Ort::Session>(*impl_->env, cfg.model_path.c_str(), opts);

        Ort::AllocatorWithDefaultOptions alloc;
        const size_t n_in = impl_->session->GetInputCount();
        for (size_t i = 0; i < n_in; ++i) {
            auto name = impl_->session->GetInputNameAllocated(i, alloc);
            impl_->input_names_storage.emplace_back(name.get());
        }
        const size_t n_out = impl_->session->GetOutputCount();
        for (size_t i = 0; i < n_out; ++i) {
            auto name = impl_->session->GetOutputNameAllocated(i, alloc);
            impl_->output_names_storage.emplace_back(name.get());
        }
        for (auto& s : impl_->input_names_storage)  impl_->input_names.push_back(s.c_str());
        for (auto& s : impl_->output_names_storage) impl_->output_names.push_back(s.c_str());

        impl_->pp.model_input_width  = cfg.input_width;
        impl_->pp.model_input_height = cfg.input_height;
        impl_->pp.conf_threshold     = cfg.conf_threshold;
        impl_->pp.nms_threshold      = cfg.nms_threshold;
        impl_->pp.max_detections     = cfg.max_detections;
        // Default Ultralytics order: 0=person, 1=bicycle, ..., the forklift class
        // is project-specific. Adjust via config in a follow-up change.
        impl_->pp.class_map[0] = domain::ObjectClass::kPerson;
        impl_->pp.class_map[1] = domain::ObjectClass::kForklift;

        LOG_INFO("onnxruntime initialized model=" << cfg.model_path);
        return {};
    } catch (const Ort::Exception& e) {
        return shared::Error{2, std::string{"ORT init failed: "} + e.what()};
    }
}

shared::Result<std::vector<domain::Detection>>
OnnxInferenceEngine::infer(const domain::Frame& frame) {
    if (!impl_->session) return shared::Error{3, "engine not initialized"};

    cv::Mat resized;
    cv::resize(frame.image, resized,
               cv::Size(impl_->cfg.input_width, impl_->cfg.input_height));

    // Convert BGR→RGB and HWC→CHW float32 blob (replaces cv::dnn::blobFromImage
    // so the DNN module is not required).
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(rgb, CV_32F, 1.0 / 255.0);

    const int H = impl_->cfg.input_height;
    const int W = impl_->cfg.input_width;
    std::vector<float> blob_data(3 * H * W);
    // Split interleaved HWC into planar CHW.
    for (int r = 0; r < H; ++r) {
        const float* row = rgb.ptr<float>(r);
        for (int c = 0; c < W; ++c) {
            blob_data[0 * H * W + r * W + c] = row[c * 3 + 0];  // R
            blob_data[1 * H * W + r * W + c] = row[c * 3 + 1];  // G
            blob_data[2 * H * W + r * W + c] = row[c * 3 + 2];  // B
        }
    }

    const std::array<int64_t, 4> shape{1, 3,
                                       impl_->cfg.input_height,
                                       impl_->cfg.input_width};
    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    try {
        Ort::Value input = Ort::Value::CreateTensor<float>(
            mem, blob_data.data(),
            blob_data.size(),
            shape.data(), shape.size());

        auto outs = impl_->session->Run(
            Ort::RunOptions{nullptr},
            impl_->input_names.data(),  &input, 1,
            impl_->output_names.data(), impl_->output_names.size());

        if (outs.empty()) return shared::Error{4, "ORT returned no outputs"};
        auto& out = outs.front();
        auto info = out.GetTensorTypeAndShapeInfo();
        const auto shape_out = info.GetShape();          // expect [1, 4+C, A]
        if (shape_out.size() != 3) {
            return shared::Error{5, "unexpected output rank"};
        }
        const std::size_t channels    = static_cast<std::size_t>(shape_out[1]);
        const std::size_t num_anchors = static_cast<std::size_t>(shape_out[2]);
        if (channels < 5) return shared::Error{6, "model has no class channels"};
        const std::size_t num_classes = channels - 4;

        PostprocessParams pp = impl_->pp;
        pp.source_width  = frame.image.cols;
        pp.source_height = frame.image.rows;

        const float* raw = out.GetTensorMutableData<float>();
        return postprocess_yolov8(raw, num_anchors, num_classes, pp);
    } catch (const Ort::Exception& e) {
        return shared::Error{7, std::string{"ORT infer failed: "} + e.what()};
    }
}

}  // namespace forklift::infrastructure::ai

#endif  // FSS_WITH_ONNXRUNTIME
