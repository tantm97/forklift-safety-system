# Transport (Alerts out)

`application::AlertPublisher` is the only port the pipeline knows about.
Today: `infrastructure::transport::WebSocketAlertPublisher`. Tomorrow:
MQTT, Kafka, HTTP webhook — same port.

## Contract

```cpp
class AlertPublisher {
public:
    virtual ~AlertPublisher() = default;
    virtual shared::Result<void> start() = 0;
    virtual shared::Result<void> publish(const domain::Alert& alert) = 0;
    virtual void stop() = 0;
};
```

- `start()` is called once from `main` before any pipelines start.
- `publish()` must be **thread-safe**; many inference workers call it
  concurrently.
- `publish()` must not block longer than ~1 ms — push onto an internal
  queue and let the I/O thread drain.
- `stop()` flushes pending alerts (best-effort, bounded by a small
  timeout) and tears down sockets.

## Serialisation

JSON, schema documented in
[`docs/architecture/websocket-api.md`](../../architecture/websocket-api.md).
Hand-rolled in `WebSocketAlertPublisher::serialize` to avoid a JSON
dependency. When the schema grows, swap in `nlohmann::json` behind the
same function signature.

## Backpressure

If all clients are slow:

- The publisher's internal queue is bounded (`drop-oldest`).
- A `WARN` log is emitted at most once per second.
- The pipeline is never slowed by transport.

## Auth

Out of scope for v1. Deploy behind a reverse proxy / VPN. File an ADR
before adding token-based auth in the WebSocket handshake.
