#!/usr/bin/env bash
# download_test_video.sh — download a forklift+person video for offline AI testing.
#
# Strategy (in order):
#   1. yt-dlp  — 90 s forklift+worker warehouse clip from YouTube (best for AI testing)
#   2. HTTP fallback — generic sample MP4 (no forklift content, pipeline smoke-test only)
#
# Usage:
#   ./scripts/download_test_video.sh          # downloads to test_data/forklift_test.mp4
#   DEST=other/path.mp4 ./scripts/download_test_video.sh

set -euo pipefail

DEST="${DEST:-test_data/forklift_test.mp4}"

# 90-second forklift+worker warehouse clip (640x360, ~5 MB).
YOUTUBE_URL="https://www.youtube.com/watch?v=14UhAiWiMq0"

# Reliable fallback: generic sample MP4 (no forklift — pipeline smoke-test only).
FALLBACK_URL="https://www.learningcontainer.com/wp-content/uploads/2020/05/sample-mp4-file.mp4"

mkdir -p "$(dirname "$DEST")"

if [ -f "$DEST" ]; then
    echo "[download_test_video] already exists: $DEST (skipping)"
    exit 0
fi

# ── Strategy 1: yt-dlp ────────────────────────────────────────────────────────
if command -v yt-dlp &>/dev/null; then
    echo "[download_test_video] using yt-dlp → $DEST"

    # Resolve a node binary for yt-dlp JS challenge solving.
    NODE_PATH=""
    for candidate in node /opt/homebrew/bin/node /usr/local/bin/node; do
        if command -v "$candidate" &>/dev/null 2>&1; then
            NODE_PATH="$(command -v "$candidate")"
            break
        fi
    done
    JS_RUNTIME_ARG=()
    if [ -n "$NODE_PATH" ]; then
        JS_RUNTIME_ARG=(--js-runtimes "node:${NODE_PATH}")
    fi

    if yt-dlp \
        "${JS_RUNTIME_ARG[@]}" \
        --format "best[ext=mp4][height<=480]/best[height<=480]/best" \
        --download-sections "*0:00-01:30" \
        --force-keyframes-at-cuts \
        --merge-output-format mp4 \
        --no-playlist \
        --quiet --progress \
        -o "$DEST" \
        "$YOUTUBE_URL" 2>&1; then
        echo "[download_test_video] done (yt-dlp) → $DEST"
        exit 0
    else
        echo "[download_test_video] yt-dlp failed, trying HTTP fallback..."
        rm -f "$DEST"
    fi
fi

# ── Strategy 2: HTTP fallback ─────────────────────────────────────────────────
echo "[download_test_video] downloading generic sample video (no forklift content)..."
if command -v curl &>/dev/null; then
    curl -L --fail --max-time 60 --progress-bar -o "$DEST" "$FALLBACK_URL"
elif command -v wget &>/dev/null; then
    wget -q --show-progress -O "$DEST" "$FALLBACK_URL"
else
    echo "[download_test_video] ERROR: neither yt-dlp, curl, nor wget found." >&2
    exit 1
fi

echo "[download_test_video] done (fallback — generic sample) → $DEST"
echo "  NOTE: This video has no forklifts. Replace with real footage for AI testing."
