'use strict';
// app.js — Forklift Safety Monitor dashboard
//
// Architecture:
//   • Fetches camera list from GET /cameras (JSON array of camera IDs).
//   • Fills a fixed 4-slot 2×2 grid; excess slots show "Offline".
//   • Each active slot subscribes to GET /stream?camera=<id> (MJPEG <img>)
//     and GET /detections?camera=<id> (SSE JSON detection data).
//   • A <canvas> overlay draws bounding boxes / zones scaled to the img
//     display size — no backend changes needed to show/hide classes.
//   • FPS is derived from the SSE message rate (detections/sec).
//   • A WebSocket connects to the alert server (:8765) for alert events.

const MAX_CAMERAS  = 4;
const MAX_ALERTS   = 100;
const WS_PORT      = '8765';
const POLL_CAMERAS_MS = 5000;  // re-poll camera list after start

// ── Overlay state ─────────────────────────────────────────────────────────────
const show = { person: true, forklift: true, zone: true };

// ── Per-slot runtime state ────────────────────────────────────────────────────
const slots = [];   // index → { el, camId, sse, lastData, fpsFrames, redraw }

// ── Entry point ───────────────────────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', () => {
    buildGrid();
    wireControls();
    connectAlerts();
    connectDetections();  // single SSE for all cameras
    loadCameras();
    // Re-poll so late-starting cameras are picked up.
    setTimeout(loadCameras, POLL_CAMERAS_MS);
});

// ── Grid construction ─────────────────────────────────────────────────────────
function buildGrid() {
    const grid = document.getElementById('camera-grid');
    for (let i = 0; i < MAX_CAMERAS; i++) {
        const cell = document.createElement('div');
        cell.className = 'camera-cell';
        cell.innerHTML = `
            <div class="cell-header">
                <span class="cell-id" id="cid-${i}">—</span>
                <span class="cell-fps" id="fps-${i}">— fps</span>
            </div>
            <div class="cell-stage">
                <img  class="cell-img"    id="img-${i}"    alt="">
                <canvas class="cell-canvas" id="cvs-${i}"></canvas>
                <div class="cell-offline" id="off-${i}">No Camera</div>
            </div>`;
        grid.appendChild(cell);

        slots.push({
            camId:     null,
            lastData:  null,
            fpsFrames: [],
            redraw:    null,
        });
    }
}

// ── Controls ──────────────────────────────────────────────────────────────────
function wireControls() {
    function wire(cbId, btnId, key) {
        const cb  = document.getElementById(cbId);
        const btn = document.getElementById(btnId);
        btn.classList.toggle(`on-${key}`, cb.checked);
        cb.addEventListener('change', () => {
            show[key] = cb.checked;
            btn.classList.toggle(`on-${key}`, cb.checked);
            slots.forEach(s => s.redraw && s.redraw());
        });
    }
    wire('toggle-person',   'btn-person',   'person');
    wire('toggle-forklift', 'btn-forklift', 'forklift');
    wire('toggle-zone',     'btn-zone',     'zone');

    document.getElementById('clear-alerts')
        .addEventListener('click', () => {
            document.getElementById('alerts').innerHTML = '';
        });
}

// ── Single multiplexed SSE for all cameras ────────────────────────────────────
// One connection handles every camera, routing by data.camera. Avoids the
// browser's HTTP/1.1 6-connection-per-origin cap (4 MJPEG + 4 SSE = 8 would
// block cam-03/04 SSE without this).
function connectDetections() {
    const sse = new EventSource('/detections');
    sse.onmessage = (e) => {
        let data;
        try { data = JSON.parse(e.data); } catch { return; }

        // Find the slot that owns this camera ID.
        const s = slots.find(sl => sl.camId === data.camera);
        if (!s) return;

        s.lastData = data;

        // FPS counter: rolling 2-second window.
        const now = performance.now();
        s.fpsFrames.push(now);
        let lo = 0;
        while (lo < s.fpsFrames.length && now - s.fpsFrames[lo] > 2000) lo++;
        if (lo > 0) s.fpsFrames.splice(0, lo);

        const idx = slots.indexOf(s);
        const fpsEl = document.getElementById(`fps-${idx}`);
        if (fpsEl) fpsEl.textContent = `${Math.round(s.fpsFrames.length / 2)} fps`;

        const img = document.getElementById(`img-${idx}`);
        const cvs = document.getElementById(`cvs-${idx}`);
        requestAnimationFrame(() => drawFrame(cvs, img, data));
    };
    sse.onerror = () => {
        // EventSource auto-reconnects; reset fps indicators.
        slots.forEach((_, i) => {
            const el = document.getElementById(`fps-${i}`);
            if (el) el.textContent = '— fps';
        });
    };
}

// ── Camera loading ────────────────────────────────────────────────────────────
async function loadCameras() {
    let cameras = [];
    try {
        const res = await fetch('/cameras');
        cameras = await res.json();
    } catch {
        return;  // server not ready yet — timeout will retry
    }
    cameras.slice(0, MAX_CAMERAS).forEach((id, idx) => {
        if (slots[idx].camId !== id) assignCamera(idx, id);
    });
}

function assignCamera(idx, camId) {
    const s = slots[idx];

    s.camId    = camId;
    s.lastData = null;
    s.fpsFrames = [];

    // Update header label.
    document.getElementById(`cid-${idx}`).textContent = camId;
    // Hide "Offline" overlay.
    document.getElementById(`off-${idx}`).style.display = 'none';

    const img = document.getElementById(`img-${idx}`);
    const cvs = document.getElementById(`cvs-${idx}`);

    // Start MJPEG stream.
    img.src = `/stream?camera=${encodeURIComponent(camId)}`;

    // Canvas redraw closure (used when toggling show/hide overlays).
    s.redraw = () => {
        if (s.lastData) drawFrame(cvs, img, s.lastData);
    };
}

// ── Canvas overlay ────────────────────────────────────────────────────────────
function drawFrame(canvas, img, data) {
    const dw = img.clientWidth;
    const dh = img.clientHeight;
    if (!dw || !dh || !data.frame_w || !data.frame_h) return;

    // Keep canvas pixel dimensions in sync with display size.
    if (canvas.width !== dw || canvas.height !== dh) {
        canvas.width  = dw;
        canvas.height = dh;
    }

    // The <img> uses object-fit: contain, so the frame is letterboxed inside the
    // element. Compute the actual rendered rect so overlay boxes line up exactly.
    const frameAspect = data.frame_w / data.frame_h;
    const elemAspect  = dw / dh;
    let renderW, renderH, offX, offY;
    if (elemAspect > frameAspect) {
        renderH = dh;
        renderW = dh * frameAspect;
        offX = (dw - renderW) / 2;
        offY = 0;
    } else {
        renderW = dw;
        renderH = dw / frameAspect;
        offX = 0;
        offY = (dh - renderH) / 2;
    }
    const sx = renderW / data.frame_w;
    const sy = renderH / data.frame_h;

    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, dw, dh);

    // ── Safety zones (drawn first, behind boxes) ──────────────────────────────
    if (show.zone && data.zones && data.zones.length) {
        ctx.save();
        ctx.strokeStyle = 'rgba(239,68,68,0.9)';
        ctx.fillStyle   = 'rgba(239,68,68,0.07)';
        ctx.lineWidth   = 1.5;
        ctx.setLineDash([7, 4]);
        for (const z of data.zones) {
            const rx = offX + z.x * sx, ry = offY + z.y * sy;
            const rw = z.w * sx, rh = z.h * sy;
            ctx.fillRect(rx, ry, rw, rh);
            ctx.strokeRect(rx, ry, rw, rh);
        }
        ctx.restore();
    }

    // ── Detection boxes ───────────────────────────────────────────────────────
    if (!data.detections || !data.detections.length) return;

    ctx.save();
    ctx.font = 'bold 11px monospace';

    for (const d of data.detections) {
        // Only person / forklift are meaningful here; skip everything else.
        if (d.class !== 'person' && d.class !== 'forklift') continue;
        if (d.class === 'person'   && !show.person)   continue;
        if (d.class === 'forklift' && !show.forklift) continue;

        const color = d.class === 'person' ? '#22c55e' : '#f97316';
        const rx = offX + d.x * sx, ry = offY + d.y * sy;
        const rw = d.w * sx, rh = d.h * sy;

        // Box.
        ctx.strokeStyle = color;
        ctx.lineWidth   = 2;
        ctx.setLineDash([]);
        ctx.strokeRect(rx, ry, rw, rh);

        // Label chip.
        const label = `${d.class} ${Math.round(d.conf * 100)}%`;
        const tw    = ctx.measureText(label).width;
        const chipY = ry > 18 ? ry - 3 : ry + rh + 15;
        ctx.fillStyle = color;
        ctx.fillRect(rx - 1, chipY - 13, tw + 7, 15);
        ctx.fillStyle = '#000';
        ctx.fillText(label, rx + 2, chipY - 1);
    }
    ctx.restore();
}

// ── Alert WebSocket ───────────────────────────────────────────────────────────
function connectAlerts() {
    const host = window.location.hostname || '127.0.0.1';
    const ws   = new WebSocket(`ws://${host}:${WS_PORT}/ws/alerts`);
    const dot  = document.getElementById('ws-status');
    const lbl  = document.getElementById('ws-label');

    ws.onopen  = () => { dot.className = 'status-dot connected';    lbl.textContent = 'connected'; };
    ws.onclose = () => { dot.className = 'status-dot disconnected'; lbl.textContent = 'reconnecting…'; setTimeout(connectAlerts, 3000); };
    ws.onerror = () => { dot.className = 'status-dot error'; };
    ws.onmessage = (e) => {
        let data;
        try { data = JSON.parse(e.data); } catch { return; }
        addAlert(data);
    };
}

function addAlert(data) {
    const list = document.getElementById('alerts');
    const time = new Date().toLocaleTimeString();
    const li   = document.createElement('li');
    li.className = 'alert-item flash';
    li.innerHTML =
        `<span class="alert-time">${time}</span>` +
        `<span class="alert-cam">${data.camera_id || '?'}</span>` +
        `<span class="alert-msg">🚨 Person in forklift zone</span>`;
    list.prepend(li);
    setTimeout(() => li.classList.remove('flash'), 500);
    while (list.children.length > MAX_ALERTS) list.removeChild(list.lastChild);
}
