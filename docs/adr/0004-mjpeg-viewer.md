# ADR-0004: Annotated MJPEG viewer + frame-stream port

**Date:**     2026-06-01
**Status:**   Accepted
**Category:** Architecture
**Change:**   add-mjpeg-viewer

## Context

Operators need to *see* what the detector sees: the live camera image with
person / forklift boxes and the computed safety zone drawn on top, for both
Test (video file) and Production (RTSP) modes. The system previously emitted
only alert JSON over WebSocket; `enable_viewer` was a no-op.

Constraints:

- The safety hot path must not be perturbed — drawing is a *view* concern and
  must never mutate the frame used for detection, nor add allocation to the
  alert path of cameras that have the viewer disabled.
- Drawing uses OpenCV (`imgproc`), which is an infrastructure dependency; it
  must not leak into `application/` or `domain/`.
- No heavyweight UI build step — the dashboard must be a static asset.

## Decision

- Add two **application ports** (no third-party types):
  - `application::FrameAnnotator` — `annotate(frame, detections, zones) → Frame`
    (returns a **cloned**, drawn frame; never mutates the input).
  - `application::FrameStreamPublisher` — `publish(frame)`; best-effort, void.
- Add two **infrastructure adapters**:
  - `infrastructure::render::OverlayFrameAnnotator` — OpenCV drawing
    (person = green, forklift = orange, zone = red, plus labels).
  - `infrastructure::transport::MjpegStreamServer` — Boost.Beast HTTP server
    that implements `FrameStreamPublisher`, keeps the latest JPEG per camera,
    and serves `/` (dashboard), `/cameras` (JSON), and
    `/stream?camera=ID` (multipart/x-mixed-replace MJPEG).
- `FrameProcessingPipeline` annotates + publishes **only** when the viewer is
  enabled *and* an annotator, stream publisher, and zone service are all wired,
  and only for cameras with `enable_viewer: true`.
- The dashboard is plain static `web/` (HTML/JS/CSS) — an `<img>` MJPEG element
  plus an alert WebSocket log. No bundler.
- The MJPEG server runs on its own port (default **8088**), separate from the
  alert WebSocket (**8765**).

## Consequences

- ✅ Operators get a live annotated view in both modes with one config flag.
- ✅ The alert path is untouched for cameras without the viewer; annotation is
  opt-in per camera.
- ✅ Drawing stays in infrastructure; the pipeline only orchestrates ports.
- ✅ The transport is swappable — `FrameStreamPublisher` could later be backed
  by WebRTC/HLS without touching the pipeline.
- ⚠️ Annotation clones the frame and JPEG-encodes it; enabling the viewer on
  every camera adds CPU. Default Production config enables it only on a subset.
- ⚠️ MJPEG is simple but bandwidth-heavy; acceptable for a few concurrent
  operator views, not for many remote clients.
