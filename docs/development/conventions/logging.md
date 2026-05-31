# Logging

We use a minimal stdio logger in `infrastructure::logging::Logger` exposed
via macros:

```cpp
#include "forklift/infrastructure/logging/Logger.h"

LOG_INFO("Pipeline started for camera=%s", camera_id.c_str());
LOG_WARN("Dropped frame, queue full (dropped=%zu)", q.dropped());
LOG_ERROR("Inference failed: %s", err.c_str());
LOG_DEBUG("Detection count=%zu", dets.size());
```

## Levels

| Level | Use for                                                       |
|-------|---------------------------------------------------------------|
| ERROR | An operation failed and we can't recover                      |
| WARN  | Degraded state: dropped frames, reconnects, throttled clients |
| INFO  | One-shot lifecycle: startup, shutdown, model loaded           |
| DEBUG | Per-frame detail. Disabled in release.                        |

## Rules

- Never log inside a tight per-frame hot loop above `DEBUG`.
- Include the `camera_id` in any per-camera log line.
- Never log PII, RTSP credentials, or the contents of the alert JSON
  payload itself.
- Use printf-style format strings; no string concatenation.

## Future

When we replace the stdio backend with `spdlog`, the call sites do not
change — `LOG_INFO` etc. are the stable contract.
