# Jetson Nano — production deployment

Install `forklift-safety-system` as a systemd service that auto-starts on boot
and restarts on failure. Targets a Jetson Nano (JetPack / Ubuntu) but works on
any Debian-based box.

## 1. Prerequisites on the Jetson

- JetPack / Ubuntu with the camera network reachable.
- **ONNX Runtime** must be available to the linker/loader. JetPack does not ship
  it by default — install the NVIDIA Jetson ONNX Runtime wheel/prebuilt libs
  **before** running the installer. (TensorRT support is a build-gated stub
  today; see [ADR-0002](../adr/0002-inference-backends.md).)
- A YOLOv8 ONNX model with the class order `0 = person, 1 = forklift`. Either
  copy yours into `models/`, or run `scripts/download_model.sh` on the Jetson.

## 2. Install

```bash
git clone <this-repo> && cd forklift-safety-system

# CPU build:
sudo ./deploy/install_jetson.sh

# CUDA execution provider (recommended on Jetson, needs CUDA-enabled ORT):
sudo ./deploy/install_jetson.sh --cuda

# Skip apt (deps already present):
sudo ./deploy/install_jetson.sh --no-deps
```

The installer (idempotent — safe to re-run) will:

1. `apt install` build + runtime deps (`build-essential cmake libopencv-dev
   libyaml-cpp-dev libboost-system-dev`).
2. Build Release (`-DFSS_ORT_WITH_CUDA=ON` when `--cuda`), tests off.
3. Create a system user `forklift` (added to the `video` group), then install
   into `/opt/forklift-safety/{forklift_safety,conf,web,models}`.
   - An **existing** `conf/system.yaml` is **kept**, never clobbered.
   - Local `models/*.onnx` are copied in.
4. Install + `enable` + `restart` the `forklift-safety.service` systemd unit.

## 3. Configure

Edit the installed config and set your real RTSP URLs and the device:

```bash
sudo nano /opt/forklift-safety/conf/system.yaml
sudo systemctl restart forklift-safety
```

Key fields for Jetson (full schema in `conf/system.yaml`):

```yaml
inference:
  model_path: models/yolov8n_forklift.onnx
  device: cuda            # cpu | cuda  — use cuda on Jetson with the --cuda build
viewer:
  enabled: true
  host: 0.0.0.0           # expose the dashboard on the LAN
cameras:
  - id: dock-01
    rtsp_url: "rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"
    enable_viewer: true   # enable only on cameras operators actually watch
```

Resource guidance for the Nano: prefer `yolov8n`, keep `queue_capacity: 4` and
`workers: 1`, and limit `enable_viewer` to a few cameras (annotation + JPEG
encode cost CPU per camera). See
[production-readiness-review](../architecture/production-readiness-review.md).

## 4. Operate

```bash
systemctl status forklift-safety          # health
journalctl -u forklift-safety -f          # live logs
sudo systemctl restart forklift-safety    # after a config edit
sudo systemctl stop forklift-safety       # stop
```

Access from another machine on the LAN:

- Dashboard: `http://<jetson-ip>:8088/`
- Alerts:    `ws://<jetson-ip>:8765/ws/alerts`

The service file (`deploy/forklift-safety.service`) runs as `forklift`,
`Restart=on-failure` (5 s), `After=network-online.target`,
`WorkingDirectory=/opt/forklift-safety` (so relative config paths resolve), with
relaxed hardening (`NoNewPrivileges`, `ProtectSystem=full`, `ProtectHome`) that
still permits CUDA/V4L access.

## 5. Upgrade

```bash
cd forklift-safety-system && git pull
sudo ./deploy/install_jetson.sh --no-deps   # rebuilds, re-syncs, restarts; keeps your config
```

## Troubleshooting

| Symptom                                   | Fix |
|-------------------------------------------|-----|
| Service keeps restarting                  | `journalctl -u forklift-safety -e`. Usually a missing model or bad RTSP URL. |
| `libonnxruntime... cannot open shared object` | ORT not installed/loadable — install the Jetson ORT and re-run with `--no-deps`. |
| CUDA requested but logs say "CPU"         | Binary not built with `--cuda`, or ORT lacks the CUDA EP. Rebuild `--cuda`. |
| No alerts despite activity                | Class map mismatch — confirm `0=person, 1=forklift` for your model ([inference.md](../development/conventions/inference.md)). |
| Dashboard unreachable from another host   | Set `viewer.host: 0.0.0.0` and open port 8088 in any firewall. |
