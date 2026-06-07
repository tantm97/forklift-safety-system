#!/usr/bin/env bash
# install_jetson.sh — build and install forklift-safety-system as a systemd
# service on a Jetson Nano (JetPack / Ubuntu) or any Debian-based Linux box.
#
# It:
#   1. installs build + runtime dependencies (apt)
#   2. builds the binary (CPU by default; pass --cuda for the GPU/CUDA EP)
#   3. installs binary + conf + web + models into /opt/forklift-safety
#   4. installs and enables the systemd service so it auto-starts on boot
#
# Usage:
#   sudo ./deploy/install_jetson.sh                # CPU build
#   sudo ./deploy/install_jetson.sh --cuda         # CUDA execution provider
#   sudo ./deploy/install_jetson.sh --no-deps      # skip apt install
#
# Re-running is safe (idempotent): it rebuilds and re-syncs files.

set -euo pipefail

PREFIX="/opt/forklift-safety"
SERVICE_USER="forklift"
WITH_CUDA=0
INSTALL_DEPS=1

for arg in "$@"; do
    case "$arg" in
        --cuda)    WITH_CUDA=1 ;;
        --no-deps) INSTALL_DEPS=0 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

if [[ $EUID -ne 0 ]]; then
    echo "Please run as root (sudo)." >&2
    exit 1
fi

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "==> forklift-safety-system installer (prefix=$PREFIX, cuda=$WITH_CUDA)"

# ── 1. Dependencies ───────────────────────────────────────────────────────────
if [[ $INSTALL_DEPS -eq 1 ]]; then
    echo "==> Installing apt dependencies"
    apt-get update
    apt-get install -y --no-install-recommends \
        build-essential cmake pkg-config git \
        libopencv-dev libyaml-cpp-dev libboost-system-dev
    # ONNX Runtime: on Jetson use the JetPack/NVIDIA wheel or prebuilt libs.
    # If libonnxruntime is not provided by your image, install it before running
    # this script (see docs/deployment/jetson-nano.md).
fi

# ── 2. Build ──────────────────────────────────────────────────────────────────
echo "==> Configuring build"
CMAKE_ARGS=(-S . -B build -DCMAKE_BUILD_TYPE=Release -DFSS_BUILD_TESTS=OFF)
if [[ $WITH_CUDA -eq 1 ]]; then
    CMAKE_ARGS+=(-DFSS_ORT_WITH_CUDA=ON)
fi
cmake "${CMAKE_ARGS[@]}"
echo "==> Building"
cmake --build build -j"$(nproc)"

# ── 3. Install files ──────────────────────────────────────────────────────────
echo "==> Creating service user '$SERVICE_USER'"
id -u "$SERVICE_USER" >/dev/null 2>&1 || useradd --system --no-create-home --shell /usr/sbin/nologin "$SERVICE_USER"
# Allow camera/GPU access where applicable.
usermod -aG video "$SERVICE_USER" || true

echo "==> Installing into $PREFIX"
install -d "$PREFIX" "$PREFIX/conf" "$PREFIX/web" "$PREFIX/models"
install -m 0755 build/forklift_safety "$PREFIX/forklift_safety"
cp -r web/. "$PREFIX/web/"
# Do not clobber an existing edited config.
if [[ ! -f "$PREFIX/conf/system.yaml" ]]; then
    install -m 0644 conf/system.yaml "$PREFIX/conf/system.yaml"
    echo "    installed default conf/system.yaml — EDIT IT with your RTSP URLs"
else
    echo "    keeping existing $PREFIX/conf/system.yaml"
fi
# Copy the model if present locally; otherwise warn.
if compgen -G "models/*.onnx" >/dev/null; then
    cp models/*.onnx "$PREFIX/models/"
else
    echo "    WARNING: no models/*.onnx found — run scripts/download_model.sh and re-copy"
fi
chown -R "$SERVICE_USER":"$SERVICE_USER" "$PREFIX"

# ── 4. systemd ────────────────────────────────────────────────────────────────
echo "==> Installing systemd service"
sed "s/^User=.*/User=$SERVICE_USER/; s/^Group=.*/Group=$SERVICE_USER/" \
    deploy/forklift-safety.service > /etc/systemd/system/forklift-safety.service
systemctl daemon-reload
systemctl enable forklift-safety.service
systemctl restart forklift-safety.service

echo ""
echo "==> Done. Service status:"
systemctl --no-pager --full status forklift-safety.service || true
echo ""
echo "Dashboard:  http://<jetson-ip>:8088/"
echo "Alerts WS:  ws://<jetson-ip>:8765/ws/alerts"
echo "Logs:       journalctl -u forklift-safety -f"
echo "Edit conf:  sudo nano $PREFIX/conf/system.yaml && sudo systemctl restart forklift-safety"
