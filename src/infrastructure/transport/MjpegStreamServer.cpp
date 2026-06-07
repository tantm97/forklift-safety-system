#include "forklift/infrastructure/transport/MjpegStreamServer.h"

#include <cstdio>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "forklift/domain/ObjectClass.h"
#include "forklift/infrastructure/logging/Logger.h"

// ─── Boost.Beast HTTP / MJPEG server ─────────────────────────────────────────
#ifdef FSS_WITH_BOOST_BEAST

#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <opencv2/imgcodecs.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

namespace forklift::infrastructure::transport {

namespace {

// ─── SharedState ─────────────────────────────────────────────────────────────
// Thread-safe store of the latest JPEG frame and detection JSON per camera.
struct SharedState {
    // ── MJPEG frames ──────────────────────────────────────────────────────────
    std::mutex                                                          frame_mu;
    std::unordered_map<std::string, std::shared_ptr<const std::string>> latest;
    std::vector<std::string>                                            camera_order;

    std::shared_ptr<const std::string> latest_for(const std::string& cam) {
        std::lock_guard<std::mutex> lk(frame_mu);
        if (cam.empty() && !camera_order.empty()) {
            auto it = latest.find(camera_order.front());
            return it == latest.end() ? nullptr : it->second;
        }
        auto it = latest.find(cam);
        return it == latest.end() ? nullptr : it->second;
    }

    std::string cameras_json() {
        std::lock_guard<std::mutex> lk(frame_mu);
        std::string out = "[";
        for (std::size_t i = 0; i < camera_order.size(); ++i) {
            out += '"' + camera_order[i] + '"';
            if (i + 1 < camera_order.size()) out += ',';
        }
        out += "]";
        return out;
    }

    void store(const std::string& cam, std::shared_ptr<const std::string> jpeg) {
        std::lock_guard<std::mutex> lk(frame_mu);
        if (latest.find(cam) == latest.end()) camera_order.push_back(cam);
        latest[cam] = std::move(jpeg);
    }

    // ── Detection SSE data ────────────────────────────────────────────────────
    struct DetEntry {
        std::string  json;
        std::uint64_t seq{0};
    };

    std::mutex                                 det_mu;
    std::unordered_map<std::string, DetEntry>  latest_dets;
    std::uint64_t                              global_det_seq{0};  // bumped on any update

    // Returns {json, seq}. seq == 0 means no data yet.
    std::pair<std::string, std::uint64_t> latest_dets_for(const std::string& cam) {
        std::lock_guard<std::mutex> lk(det_mu);
        auto it = latest_dets.find(cam);
        if (it == latest_dets.end()) return {"", 0};
        return {it->second.json, it->second.seq};
    }

    // Returns concatenated "data: <json>\n\n" for all cameras, and the global
    // sequence number. Used by the all-cameras multiplexed SSE stream so the
    // browser only needs one connection regardless of camera count.
    std::pair<std::string, std::uint64_t> latest_dets_all() {
        std::lock_guard<std::mutex> lk(det_mu);
        std::string out;
        for (const auto& [cam, entry] : latest_dets) {
            if (entry.json.empty()) continue;
            out += "data: " + entry.json + "\n\n";
        }
        return {out, global_det_seq};
    }

    void store_dets(const std::string& cam, std::string json) {
        std::lock_guard<std::mutex> lk(det_mu);
        auto& e = latest_dets[cam];
        e.json = std::move(json);
        ++e.seq;
        ++global_det_seq;
    }
};

// ─── Helpers ─────────────────────────────────────────────────────────────────
std::string query_param(const std::string& target, const std::string& key) {
    const auto qpos = target.find('?');
    if (qpos == std::string::npos) return {};
    std::string query = target.substr(qpos + 1);
    std::istringstream ss(query);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        const auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        if (pair.substr(0, eq) == key) return pair.substr(eq + 1);
    }
    return {};
}

std::string path_only(const std::string& target) {
    const auto qpos = target.find('?');
    return qpos == std::string::npos ? target : target.substr(0, qpos);
}

std::string content_type_for(const std::string& path) {
    auto ends = [&](const char* s) {
        const std::string suf = s;
        return path.size() >= suf.size() &&
               path.compare(path.size() - suf.size(), suf.size(), suf) == 0;
    };
    if (ends(".html")) return "text/html; charset=utf-8";
    if (ends(".js"))   return "application/javascript; charset=utf-8";
    if (ends(".css"))  return "text/css; charset=utf-8";
    if (ends(".png"))  return "image/png";
    if (ends(".svg"))  return "image/svg+xml";
    if (ends(".ico"))  return "image/x-icon";
    return "application/octet-stream";
}

}  // namespace

// ─── Impl ─────────────────────────────────────────────────────────────────────

struct MjpegStreamServer::Impl {
    ViewerServerConfig cfg;

    net::io_context ioc{1};
    tcp::acceptor   acceptor{ioc};
    std::thread     io_thread;
    bool            running{false};

    std::shared_ptr<SharedState> state{std::make_shared<SharedState>()};

    void start_accept();
};

// ─── HttpSession ──────────────────────────────────────────────────────────────
// One per connection. Serves one-shot HTTP responses, open-ended MJPEG streams,
// or persistent SSE detection streams. Lives only on the io_context thread.

namespace {

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket&& sock, std::shared_ptr<SharedState> state, ViewerServerConfig cfg)
        : stream_(std::move(sock)), state_(std::move(state)), cfg_(std::move(cfg)),
          timer_(stream_.get_executor()) {}

    void run() { do_read(); }

private:
    void do_read() {
        req_ = {};
        stream_.expires_after(std::chrono::seconds(30));
        http::async_read(stream_, buffer_, req_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) return;
                self->route();
            });
    }

    void route() {
        const std::string target = std::string(req_.target());
        const std::string path   = path_only(target);

        if (path == "/stream") {
            camera_ = query_param(target, "camera");
            start_mjpeg();
            return;
        }
        if (path == "/detections") {
            camera_ = query_param(target, "camera");
            start_sse();
            return;
        }
        if (path == "/cameras") {
            send_string(state_->cameras_json(), "application/json");
            return;
        }
        if (path == "/" || path == "/index.html") {
            send_file("index.html");
            return;
        }
        if (path.find("..") != std::string::npos) {
            send_status(http::status::bad_request, "bad path");
            return;
        }
        send_file(path.substr(1));
    }

    // ── One-shot responses ──────────────────────────────────────────────────
    void send_string(const std::string& body, const std::string& ctype) {
        auto res = std::make_shared<http::response<http::string_body>>(
            http::status::ok, req_.version());
        res->set(http::field::server, "forklift-safety");
        res->set(http::field::content_type, ctype);
        res->set(http::field::access_control_allow_origin, "*");
        res->keep_alive(false);
        res->body() = body;
        res->prepare_payload();
        http::async_write(stream_, *res,
            [self = shared_from_this(), res](beast::error_code, std::size_t) {
                self->stream_.socket().shutdown(tcp::socket::shutdown_send,
                                                self->ignored_ec_);
            });
    }

    void send_status(http::status status, const std::string& msg) {
        auto res = std::make_shared<http::response<http::string_body>>(
            status, req_.version());
        res->set(http::field::server, "forklift-safety");
        res->set(http::field::content_type, "text/plain; charset=utf-8");
        res->keep_alive(false);
        res->body() = msg;
        res->prepare_payload();
        http::async_write(stream_, *res,
            [self = shared_from_this(), res](beast::error_code, std::size_t) {
                self->stream_.socket().shutdown(tcp::socket::shutdown_send,
                                                self->ignored_ec_);
            });
    }

    void send_file(const std::string& rel) {
        const std::string full = cfg_.web_root + "/" + rel;
        std::ifstream in(full, std::ios::binary);
        if (!in) {
            send_status(http::status::not_found, "not found: " + rel);
            return;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        send_string(ss.str(), content_type_for(rel));
    }

    // ── MJPEG streaming ──────────────────────────────────────────────────────
    void start_mjpeg() {
        stream_.expires_never();
        const std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Server: forklift-safety\r\n"
            "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            "Pragma: no-cache\r\n"
            "Connection: close\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
            "\r\n";
        out_ = header;
        net::async_write(stream_, net::buffer(out_),
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) return;
                self->mjpeg_tick();
            });
    }

    void mjpeg_tick() {
        auto jpeg = state_->latest_for(camera_);
        if (jpeg && jpeg != last_sent_) {
            last_sent_ = jpeg;
            out_  = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: ";
            out_ += std::to_string(jpeg->size());
            out_ += "\r\n\r\n";
            out_ += *jpeg;
            out_ += "\r\n";
            net::async_write(stream_, net::buffer(out_),
                [self = shared_from_this()](beast::error_code ec, std::size_t) {
                    if (ec) return;
                    self->mjpeg_arm_timer();
                });
        } else {
            mjpeg_arm_timer();
        }
    }

    void mjpeg_arm_timer() {
        const int fps = cfg_.target_fps > 0 ? cfg_.target_fps : 15;
        timer_.expires_after(std::chrono::milliseconds(1000 / fps));
        timer_.async_wait([self = shared_from_this()](beast::error_code ec) {
            if (ec) return;
            self->mjpeg_tick();
        });
    }

    // ── SSE detection stream ──────────────────────────────────────────────────
    void start_sse() {
        stream_.expires_never();
        const std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Server: forklift-safety\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Connection: close\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n";
        out_ = header;
        net::async_write(stream_, net::buffer(out_),
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) return;
                self->sse_tick();
            });
    }

    void sse_tick() {
        // When camera_ is empty, multiplex all cameras into one stream so the
        // browser only needs one connection (avoiding the HTTP/1.1 6-connection
        // per-origin limit when many cameras are configured).
        if (camera_.empty()) {
            auto [payload, seq] = state_->latest_dets_all();
            if (!payload.empty() && seq != sse_last_seq_) {
                sse_last_seq_ = seq;
                out_ = payload;
                net::async_write(stream_, net::buffer(out_),
                    [self = shared_from_this()](beast::error_code ec, std::size_t) {
                        if (ec) return;
                        self->sse_arm_timer();
                    });
            } else {
                sse_arm_timer();
            }
            return;
        }
        auto [json, seq] = state_->latest_dets_for(camera_);
        if (!json.empty() && seq != sse_last_seq_) {
            sse_last_seq_ = seq;
            out_ = "data: " + json + "\n\n";
            net::async_write(stream_, net::buffer(out_),
                [self = shared_from_this()](beast::error_code ec, std::size_t) {
                    if (ec) return;
                    self->sse_arm_timer();
                });
        } else {
            sse_arm_timer();
        }
    }

    void sse_arm_timer() {
        // Poll at 2× target FPS to minimise detection display latency.
        const int fps = cfg_.target_fps > 0 ? cfg_.target_fps : 15;
        timer_.expires_after(std::chrono::milliseconds(500 / fps));
        timer_.async_wait([self = shared_from_this()](beast::error_code ec) {
            if (ec) return;
            self->sse_tick();
        });
    }

    beast::tcp_stream                  stream_;
    beast::flat_buffer                 buffer_;
    http::request<http::string_body>   req_;
    std::shared_ptr<SharedState>       state_;
    ViewerServerConfig                 cfg_;
    net::steady_timer                  timer_;
    std::string                        camera_;
    std::string                        out_;
    std::shared_ptr<const std::string> last_sent_;   // MJPEG dedup
    std::uint64_t                      sse_last_seq_{0};
    beast::error_code                  ignored_ec_;
};

}  // namespace

void MjpegStreamServer::Impl::start_accept() {
    acceptor.async_accept([this](beast::error_code ec, tcp::socket sock) {
        if (!running) return;
        if (!ec) {
            std::make_shared<HttpSession>(std::move(sock), state, cfg)->run();
        }
        start_accept();
    });
}

// ─── Public API ───────────────────────────────────────────────────────────────

MjpegStreamServer::MjpegStreamServer(ViewerServerConfig cfg)
    : impl_(std::make_unique<Impl>()) {
    impl_->cfg = std::move(cfg);
}

MjpegStreamServer::~MjpegStreamServer() { stop(); }

shared::Result<void> MjpegStreamServer::start() {
    if (impl_->running) return {};
    try {
        const auto addr = net::ip::make_address(impl_->cfg.host);
        tcp::endpoint ep{addr, impl_->cfg.port};
        impl_->acceptor.open(ep.protocol());
        impl_->acceptor.set_option(net::socket_base::reuse_address(true));
        impl_->acceptor.bind(ep);
        impl_->acceptor.listen(net::socket_base::max_listen_connections);
    } catch (const std::exception& e) {
        return shared::Error{30, std::string{"mjpeg bind failed: "} + e.what()};
    }

    impl_->running = true;
    impl_->start_accept();
    impl_->io_thread = std::thread([this] { impl_->ioc.run(); });

    LOG_INFO("mjpeg viewer started host=" << impl_->cfg.host
             << " port=" << impl_->cfg.port << " web_root=" << impl_->cfg.web_root);
    return {};
}

void MjpegStreamServer::stop() {
    if (!impl_->running) return;
    impl_->running = false;
    impl_->ioc.stop();
    if (impl_->io_thread.joinable()) impl_->io_thread.join();
    LOG_INFO("mjpeg viewer stopped");
}

void MjpegStreamServer::publish(const domain::Frame& frame) {
    if (!impl_->running || frame.image.empty()) return;

    std::vector<int> params{cv::IMWRITE_JPEG_QUALITY,
                            impl_->cfg.jpeg_quality > 0 ? impl_->cfg.jpeg_quality : 75};
    std::vector<uchar> buf;
    if (!cv::imencode(".jpg", frame.image, buf, params)) return;

    auto jpeg = std::make_shared<const std::string>(buf.begin(), buf.end());
    impl_->state->store(frame.camera_id, std::move(jpeg));
}

void MjpegStreamServer::publish_detections(
        const std::string&                     camera_id,
        const std::vector<domain::Detection>&  detections,
        const std::vector<domain::SafetyZone>& zones,
        int                                    frame_w,
        int                                    frame_h) {
    if (!impl_->running) return;

    // Build JSON without a library dependency on the hot path.
    std::string json;
    json.reserve(512);
    json  = "{\"camera\":\"";
    json += camera_id;
    json += "\",\"frame_w\":";
    json += std::to_string(frame_w);
    json += ",\"frame_h\":";
    json += std::to_string(frame_h);
    json += ",\"detections\":[";
    for (std::size_t i = 0; i < detections.size(); ++i) {
        if (i > 0) json += ',';
        const auto& d = detections[i];
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", d.confidence);
        json += "{\"class\":\"";
        json += std::string(domain::to_string(d.cls));
        json += "\",\"x\":";
        json += std::to_string(static_cast<int>(d.box.x));
        json += ",\"y\":";
        json += std::to_string(static_cast<int>(d.box.y));
        json += ",\"w\":";
        json += std::to_string(static_cast<int>(d.box.width));
        json += ",\"h\":";
        json += std::to_string(static_cast<int>(d.box.height));
        json += ",\"conf\":";
        json += buf;
        json += '}';
    }
    json += "],\"zones\":[";
    for (std::size_t i = 0; i < zones.size(); ++i) {
        if (i > 0) json += ',';
        const auto& z = zones[i];
        json += "{\"x\":";
        json += std::to_string(static_cast<int>(z.zone_box.x));
        json += ",\"y\":";
        json += std::to_string(static_cast<int>(z.zone_box.y));
        json += ",\"w\":";
        json += std::to_string(static_cast<int>(z.zone_box.width));
        json += ",\"h\":";
        json += std::to_string(static_cast<int>(z.zone_box.height));
        json += '}';
    }
    json += "]}";

    impl_->state->store_dets(camera_id, std::move(json));
}

}  // namespace forklift::infrastructure::transport

// ─── Stub fallback (no Boost.Beast) ──────────────────────────────────────────
#else

namespace forklift::infrastructure::transport {

struct MjpegStreamServer::Impl {
    ViewerServerConfig cfg;
    bool               started{false};
};

MjpegStreamServer::MjpegStreamServer(ViewerServerConfig cfg)
    : impl_(std::make_unique<Impl>()) {
    impl_->cfg = std::move(cfg);
}

MjpegStreamServer::~MjpegStreamServer() { stop(); }

shared::Result<void> MjpegStreamServer::start() {
    impl_->started = true;
    LOG_WARN("mjpeg viewer is a no-op stub (built without FSS_WITH_BOOST_BEAST)");
    return {};
}

void MjpegStreamServer::stop() {
    if (!impl_->started) return;
    impl_->started = false;
    LOG_INFO("mjpeg viewer stopped");
}

void MjpegStreamServer::publish(const domain::Frame& /*frame*/) {}

void MjpegStreamServer::publish_detections(
        const std::string& /*camera_id*/,
        const std::vector<domain::Detection>& /*detections*/,
        const std::vector<domain::SafetyZone>& /*zones*/,
        int /*frame_w*/,
        int /*frame_h*/) {}

}  // namespace forklift::infrastructure::transport

#endif  // FSS_WITH_BOOST_BEAST
