---
applyTo: '**/infrastructure/transport/**/*.{h,cpp}'
description: 'Alert transport adapter conventions (WebSocket / AlertPublisher port)'
---

> **Source of truth:** [`docs/development/conventions/transport.md`](../../docs/development/conventions/transport.md)

Read the full transport document before editing transport adapters.

Key rules (quick reference):
- All transport adapters implement `application::AlertPublisher` (`start`, `publish`, `stop`).
- `publish()` MUST be thread-safe — many inference workers call it concurrently.
- `publish()` must not block > ~1 ms. Push onto an internal bounded queue; drain on the I/O thread.
- Internal queue uses drop-oldest when full; emit a `LOG_WARN` at most once per second.
- JSON serialisation: hand-rolled in `serialize()` static method. Swap for `nlohmann::json` only when schema grows complex — do not add the dep until then.
- Alert schema is versioned via `schema_version` field. Bump on every breaking change and update `docs/architecture/websocket-api.md`.
- No auth in v1 — deploy behind reverse proxy/VPN. File an ADR before adding token-based handshake auth.
- `stop()` must flush pending alerts (best-effort, bounded timeout) before tearing down sockets.
