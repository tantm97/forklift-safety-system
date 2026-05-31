---
applyTo: '**/*.{h,cpp}'
description: 'Security: secrets, credentials, PII, transport hardening'
---

Key rules (quick reference):
- Never commit RTSP credentials, API keys, or passwords. `conf/system.yaml` must be in `.gitignore` for production deployments; use `conf/system.local.yaml` (already gitignored).
- Never log RTSP URLs (contain credentials), JWT tokens, or PII.
- `LOG_DEBUG` lines containing sensitive data must be guarded by a compile-time debug macro, not a runtime flag.
- Alert JSON (`WebSocketAlertPublisher::serialize`) must never include RTSP stream URLs or internal hostnames.
- File paths used for model loading must come from config, not from user/network input. Validate that the path does not escape the `models/` directory.
- WebSocket server: no auth in v1 — document clearly in `docs/architecture/websocket-api.md`. CORS and TLS are the responsibility of the reverse proxy.
- RTSP credentials in config → `chmod 600 conf/system.yaml` instruction in `README.md` and `local-setup.md`.
