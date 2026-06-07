# WebSocket Alert API

> This is the **alert JSON** channel. For the live annotated video dashboard
> (a separate server on port 8088), see
> [`ui-streaming.md`](ui-streaming.md).

## Endpoint

```
ws://<host>:<port><path>
```

Defaults (configurable in `conf/system.yaml`):

| Key             | Default       |
|-----------------|---------------|
| `websocket.host`| `0.0.0.0`     |
| `websocket.port`| `8765`        |
| `websocket.path`| `/ws/alerts`  |

## Connection lifecycle

- Clients connect any time; the server keeps no per-client state beyond the
  socket handle.
- No subscribe message is required — every connected client receives every
  alert.
- The server never closes the connection unless it is shutting down.

## Message: `person_near_forklift`

Direction: server → client. Encoded as a single JSON object per frame.

```json
{
  "schema_version": 1,
  "alert_id": "dock-01:F7:P3:1700000000123456789",
  "type": "person_near_forklift",
  "severity": "warning",
  "camera_id": "dock-01",
  "timestamp_ms": 1700000000123,
  "person_track_id": 3,
  "forklift_track_id": 7,
  "person_box":      { "x": 412.0, "y": 308.0, "w":  76.0, "h": 188.0 },
  "forklift_box":    { "x": 280.0, "y": 320.0, "w": 240.0, "h": 180.0 },
  "safety_zone_box": { "x": 160.0, "y": 230.0, "w": 480.0, "h": 360.0 },
  "distance_px": 0.0
}
```

| Field               | Type   | Notes |
|---------------------|--------|-------|
| `schema_version`    | int    | Bumped on every breaking change |
| `alert_id`          | string | Stable within a single process; `<camera>:F<f>:P<p>:<ns>` |
| `type`              | string | Currently only `person_near_forklift` |
| `severity`          | string | `info` · `warning` · `critical` |
| `camera_id`         | string | As configured in `cameras[].id` |
| `timestamp_ms`      | int    | Unix epoch ms, server clock |
| `person_track_id`   | int    | -1 if tracker disabled |
| `forklift_track_id` | int    | -1 if tracker disabled |
| `*_box`             | object | Pixel coords: top-left `x,y` + `w,h` |
| `distance_px`       | float  | 0 when the person box overlaps the safety zone |

## Versioning

The `schema_version` field allows clients to negotiate at parse-time. Any
breaking change MUST:

1. Bump `schema_version`
2. Update this document
3. Add an ADR
4. Note the migration step in the change's `tasks.md`
