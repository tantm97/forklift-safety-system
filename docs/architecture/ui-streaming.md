# UI / streaming — the annotated MJPEG dashboard

The operator UI shows each camera's live image with detection boxes and the
safety zone drawn on top, plus a running log of alerts. It works identically in
**Test** mode (video file) and **Production** mode (RTSP). Design rationale:
[ADR-0004](../adr/0004-mjpeg-viewer.md).

## How it fits the architecture

```
                              ┌─ RiskDetectionService ─► Alert ─► AlertPublisher (WS :8765)
Frame ─► InferenceEngine ─► Detection[] ─┤
                              └─ SafetyZoneService ─► SafetyZone[]
                                         │
                 (viewer enabled only) ──┼─► FrameAnnotator.annotate(frame, dets, zones)
                                         │        └─► OverlayFrameAnnotator (OpenCV draw on a CLONE)
                                         └─► FrameStreamPublisher.publish(annotated)
                                                  └─► MjpegStreamServer (HTTP :8088)
```

Two application **ports** keep OpenCV and HTTP out of the pipeline:

| Port                                   | Contract                                            | Adapter |
|----------------------------------------|-----------------------------------------------------|---------|
| `application::FrameAnnotator`          | `annotate(frame, detections, zones) → Frame` (clone)| `infrastructure::render::OverlayFrameAnnotator` |
| `application::FrameStreamPublisher`    | `publish(frame)` — best-effort, void                | `infrastructure::transport::MjpegStreamServer` |

The annotator returns a **cloned** frame, so the safety path never sees drawn
pixels. Annotation + publishing happen only when the viewer is enabled and all
three of annotator / stream publisher / zone service are wired, and only for
cameras with `enable_viewer: true`. Cameras with the viewer off pay nothing.

## Drawing legend

| Element         | Colour  |
|-----------------|---------|
| Person box      | green   |
| Forklift box    | orange  |
| Safety zone     | red     |
| Confidence label| per-box |

## HTTP endpoints (`MjpegStreamServer`, default port 8088)

| Method / path             | Returns                                             |
|---------------------------|-----------------------------------------------------|
| `GET /`                   | dashboard (`web_root/index.html`)                   |
| `GET /<asset>`            | static asset from `web_root` (`app.js`, `style.css`)|
| `GET /cameras`            | JSON array of camera ids that have a live frame     |
| `GET /stream?camera=<id>` | `multipart/x-mixed-replace` MJPEG stream            |

The server runs one `io_context` on a dedicated thread (mirrors
`WebSocketAlertPublisher`). `publish()` JPEG-encodes the latest frame per camera
into a shared, mutex-protected buffer; each stream session polls that buffer on a
timer capped at `target_fps` and pushes multipart parts. Frames are **dropped**
under back-pressure — this is a convenience view, not the safety path. Boost.Beast
sits behind a PIMPL; when Beast is absent a log-only stub is compiled instead.

## The dashboard (`web/`)

Plain static assets — **no build step**:

- `index.html` — camera selector + MJPEG `<img>` + alert log
- `app.js` — `loadCameras()` (`/cameras`), `setStream()` (points the `<img>` at
  `/stream?camera=ID`), `connectAlerts()` (alert WebSocket), `addAlert()`
- `style.css`

The alert log connects to the alert WebSocket, which is a **separate** server on
port 8765 (`ws://<host>:8765/ws/alerts`). The two ports are independent:

| Concern          | Server                    | Default port |
|------------------|---------------------------|--------------|
| Annotated video  | `MjpegStreamServer`       | 8088         |
| Alert JSON       | `WebSocketAlertPublisher` | 8765         |

Alert message schema: [`websocket-api.md`](websocket-api.md).

## Configuration

```yaml
viewer:
  enabled: true        # master switch for the MJPEG server
  host: 0.0.0.0        # 127.0.0.1 for laptop-only; 0.0.0.0 to expose on the LAN
  port: 8088
  web_root: web
  jpeg_quality: 75     # 1..100
  target_fps: 15       # stream cadence cap

cameras:
  - id: dock-01
    enable_viewer: true   # per-camera opt-in (annotation cost is per camera)
```

## Performance notes

Annotation clones the frame and JPEG-encodes it per published frame. Enable the
viewer only on the cameras an operator actually watches; lower `jpeg_quality` and
`target_fps` to cut CPU and bandwidth. MJPEG is intentionally simple and is fine
for a handful of operator views, not for many remote clients — swap the
`FrameStreamPublisher` adapter (e.g. WebRTC/HLS) if you need scale.
