# Naming Conventions

| Kind                   | Style              | Example                            |
|------------------------|--------------------|------------------------------------|
| Namespace              | snake_case         | `forklift::application`            |
| Type / class / struct  | PascalCase         | `FrameProcessingPipeline`          |
| Function / method      | snake_case         | `compute_safety_zone()`            |
| Member variable        | snake_case + `_`   | `frame_queue_`                     |
| Local variable         | snake_case         | `forklift_box`                     |
| Constant / constexpr   | kCamelCase         | `kDefaultCooldownMs`               |
| Enum value             | kCamelCase         | `OverflowPolicy::kDropOldestWhenFull` |
| Macro                  | UPPER_SNAKE        | `FORKLIFT_DOMAIN_FRAME_H_`         |
| File: header           | PascalCase.h       | `BoundingBox.h`                    |
| File: source           | PascalCase.cpp     | `BoundingBox.cpp`                  |
| File: test             | snake_case_test.cpp| `bounding_box_test.cpp`            |
| Folder                 | snake_case         | `include/forklift/infrastructure/` |
| CMake target           | snake_case         | `forklift_core`, `forklift_safety` |
| Config key (YAML)      | snake_case         | `alert_cooldown_ms`                |

## Suffixes / prefixes

- Pointer or owning handle: no Hungarian prefix.
- Boolean: `is_*`, `has_*`, `should_*`.
- Counters: `*_count`, durations: `*_ms` / `*_us` / `*_ns`.
- Abbreviations: keep acronyms PascalCased (`RtspCameraSource`, not
  `RTSPCameraSource`).
