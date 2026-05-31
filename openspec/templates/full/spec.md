# Spec: {{capability-name}}

## Overview

<!-- What the capability is. Where it lives in the pipeline. -->

## Requirements

### Functional Requirements

1.
2.
3.

### Non-Functional Requirements

- Throughput budget: ≥ 15 FPS per camera at <model_input>
- Latency budget: end-to-end frame → alert ≤ <X> ms (p95)
- Memory budget: ≤ <X> MB per camera pipeline
- Thread-safety: <describe contract>

## Interface

### C++ public headers

```cpp
// include/forklift/<layer>/<Name>.h
namespace forklift::<layer> {

class <Name> {
 public:
    // ...
};

}  // namespace forklift::<layer>
```

### Configuration keys (conf/system.yaml)

```yaml
<section>:
  <key>: <default>     # description
```

### WebSocket alert schema (only if changed)

```json
{
  "alert_id": "string",
  "type": "string",
  "...": "..."
}
```

## Behavior

### Happy Path

<!-- Frame-by-frame walk-through of the pipeline path the new capability touches. -->

### Error Handling

| Trigger | Error code / log | Recovery |
|---------|------------------|----------|
|         |                  |          |

### Edge Cases

- Camera reconnect during inference burst
- Model returns zero detections for N consecutive frames
- Queue overflow → drop-oldest counter increments
- Multiple persons inside the same zone in one frame

## Acceptance Criteria

- [ ]
- [ ]
- [ ]
