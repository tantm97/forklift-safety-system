# Testing

## Layout

```
tests/
├── CMakeLists.txt
├── test_harness.h        # zero-dep TEST_CASE / EXPECT_* macros
├── test_main.cpp
├── domain/               # microsecond unit tests, no deps
└── application/          # use-case tests with fakes
```

Run:

```bash
cmake -S . -B build -DFSS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## What to test

- **Domain layer**: 100 % branch coverage. Pure functions, no excuses.
- **Application services**: full coverage via fake `InferenceEngine` and
  fake `AlertPublisher`. No real RTSP, no real model.
- **Infrastructure adapters**: smoke tests behind a build flag — they need
  real ORT / OpenCV.
- **Pipeline orchestration**: deterministic clocks + scripted fake source.

## Test naming

`tests/<layer>/<noun>_test.cpp`, e.g. `safety_zone_service_test.cpp`. Each
`TEST_CASE` name describes the behaviour, not the function:

```cpp
TEST_CASE("Alert is suppressed within cooldown window") { ... }
```

## Fakes vs mocks

Hand-rolled fakes that implement the port interfaces. We do not use a
mocking framework — fakes compose better and read clearer.

```cpp
class FakeInferenceEngine : public application::InferenceEngine {
public:
    std::vector<Detection> scripted_;
    Result<void> initialize() override { return Result<void>::ok(); }
    Result<std::vector<Detection>> infer(const Frame&) override {
        return Result<std::vector<Detection>>::ok(scripted_);
    }
    std::string backend_name() const override { return "fake"; }
};
```

## CI gate

CI must:

1. Build with `-DFSS_WITH_ONNXRUNTIME=OFF` (default) and `-DFSS_BUILD_TESTS=ON`.
2. Run `ctest`.
3. Re-build with `-DFSS_WITH_ONNXRUNTIME=ON` if ORT is present in the image.
