#include "forklift/infrastructure/transport/WebSocketAlertPublisher.h"

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#include "forklift/infrastructure/logging/Logger.h"

// ─── Boost.Beast real WebSocket server ────────────────────────────────────────
#ifdef FSS_WITH_BOOST_BEAST

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;
namespace ws    = beast::websocket;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

namespace forklift::infrastructure::transport {

// ─── Session ──────────────────────────────────────────────────────────────────
// One instance per connected client. Accessed only from the io_context thread.

class Session : public std::enable_shared_from_this<Session> {
public:
    using RemoveFn = std::function<void(Session*)>;

    explicit Session(tcp::socket sock, RemoveFn on_close)
        : ws_(std::move(sock)), on_close_(std::move(on_close)) {}

    void start() {
        ws_.set_option(ws::stream_base::timeout::suggested(beast::role_type::server));
        ws_.set_option(ws::stream_base::decorator([](ws::response_type& res) {
            res.set(beast::http::field::server, "forklift-safety/1.0");
        }));
        ws_.async_accept([self = shared_from_this()](beast::error_code ec) {
            if (ec) {
                LOG_WARN("ws handshake error: " << ec.message());
                self->on_close_(self.get());
                return;
            }
            self->ready_ = true;
            LOG_DEBUG("ws client connected");
        });
    }

    void send(std::shared_ptr<const std::string> payload) {
        if (!ready_) return;
        pending_.push_back(std::move(payload));
        if (!writing_) do_write();
    }

private:
    void do_write() {
        if (pending_.empty()) { writing_ = false; return; }
        writing_ = true;
        auto msg = pending_.front();
        pending_.pop_front();
        ws_.async_write(net::buffer(*msg),
            [self = shared_from_this(), msg](beast::error_code ec, std::size_t) {
                (void)msg;
                if (ec) {
                    LOG_DEBUG("ws write closed: " << ec.message());
                    self->on_close_(self.get());
                    return;
                }
                self->do_write();
            });
    }

    ws::stream<beast::tcp_stream>                     ws_;
    RemoveFn                                           on_close_;
    std::deque<std::shared_ptr<const std::string>>    pending_;
    bool writing_{false};
    bool ready_{false};
};

// ─── Impl ─────────────────────────────────────────────────────────────────────

struct WebSocketAlertPublisher::Impl {
    WebSocketServerConfig cfg;

    // Alert queue — written from any thread, drained on io_thread only.
    using Clock = std::chrono::steady_clock;
    std::mutex              queue_mu;
    std::deque<std::string> queue;
    Clock::time_point       last_drop_warn{Clock::now() - std::chrono::seconds{10}};
    static constexpr std::size_t kMaxQueue{256};

    // ASIO / Beast (all session access is on io_thread only).
    net::io_context                    ioc{1};
    tcp::acceptor                      acceptor{ioc};
    std::set<std::shared_ptr<Session>> sessions;

    std::thread io_thread;
    bool        running{false};

    void start_accept() {
        acceptor.async_accept([this](beast::error_code ec, tcp::socket sock) {
            if (!running) return;
            if (!ec) {
                auto session = std::make_shared<Session>(
                    std::move(sock),
                    [this](Session* s) { remove_session(s); });
                sessions.insert(session);
                session->start();
                LOG_DEBUG("ws client accepted, connected=" << sessions.size());
            }
            start_accept();
        });
    }

    void remove_session(Session* raw) {
        for (auto it = sessions.begin(); it != sessions.end(); ++it) {
            if (it->get() == raw) { sessions.erase(it); break; }
        }
        LOG_DEBUG("ws client removed, connected=" << sessions.size());
    }

    // Must be called only from the io_thread.
    void do_drain() {
        std::deque<std::string> local;
        {
            std::lock_guard<std::mutex> lk(queue_mu);
            local.swap(queue);
        }
        for (auto& payload : local) {
            auto shared = std::make_shared<const std::string>(std::move(payload));
            for (auto& session : sessions) session->send(shared);
        }
    }
};

// ─── Public API ───────────────────────────────────────────────────────────────

WebSocketAlertPublisher::WebSocketAlertPublisher(WebSocketServerConfig cfg)
    : impl_(std::make_unique<Impl>()) {
    impl_->cfg = std::move(cfg);
}

WebSocketAlertPublisher::~WebSocketAlertPublisher() { stop(); }

shared::Result<void> WebSocketAlertPublisher::start() {
    if (impl_->running) return {};
    try {
        const auto addr = net::ip::make_address(impl_->cfg.host);
        tcp::endpoint ep{addr, impl_->cfg.port};
        impl_->acceptor.open(ep.protocol());
        impl_->acceptor.set_option(net::socket_base::reuse_address(true));
        impl_->acceptor.bind(ep);
        impl_->acceptor.listen(net::socket_base::max_listen_connections);
    } catch (const std::exception& e) {
        return shared::Error{10, std::string{"ws bind failed: "} + e.what()};
    }

    impl_->running = true;
    impl_->start_accept();
    impl_->io_thread = std::thread([this] { impl_->ioc.run(); });

    LOG_INFO("websocket publisher started host=" << impl_->cfg.host
             << " port=" << impl_->cfg.port << " path=" << impl_->cfg.path);
    return {};
}

void WebSocketAlertPublisher::stop() {
    if (!impl_->running) return;
    impl_->running = false;

    // Best-effort flush: drain queue, then stop the io_context.
    net::post(impl_->ioc, [this] { impl_->do_drain(); });
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    impl_->ioc.stop();

    if (impl_->io_thread.joinable()) impl_->io_thread.join();
    LOG_INFO("websocket publisher stopped");
}

shared::Result<void> WebSocketAlertPublisher::publish(const domain::Alert& alert) {
    const std::string payload = serialize(alert);
    {
        std::lock_guard<std::mutex> lk(impl_->queue_mu);
        if (impl_->queue.size() >= Impl::kMaxQueue) {
            impl_->queue.pop_front();
            const auto now = Impl::Clock::now();
            if (now - impl_->last_drop_warn >= std::chrono::seconds{1}) {
                LOG_WARN("ws queue full — dropping oldest alert camera=" << alert.camera_id);
                impl_->last_drop_warn = now;
            }
        }
        impl_->queue.push_back(payload);
    }
    net::post(impl_->ioc, [this] { impl_->do_drain(); });
    return {};
}

// ─── Serialization ────────────────────────────────────────────────────────────

std::string WebSocketAlertPublisher::serialize(const domain::Alert& alert) {
    using namespace std::chrono;
    const auto epoch_ms = duration_cast<milliseconds>(
                              alert.timestamp.time_since_epoch()).count();

    auto box = [](const domain::BoundingBox& b) {
        std::ostringstream o;
        o << "{\"x\":" << b.x << ",\"y\":" << b.y
          << ",\"w\":" << b.width << ",\"h\":" << b.height << "}";
        return o.str();
    };

    std::ostringstream o;
    o << "{"
      << "\"schema_version\":1,"
      << "\"alert_id\":\""        << alert.alert_id << "\","
      << "\"type\":\""            << domain::to_string(alert.type) << "\","
      << "\"severity\":\""        << domain::to_string(alert.severity) << "\","
      << "\"camera_id\":\""       << alert.camera_id << "\","
      << "\"timestamp_ms\":"      << epoch_ms << ","
      << "\"person_track_id\":"   << alert.person_track_id << ","
      << "\"forklift_track_id\":" << alert.forklift_track_id << ","
      << "\"person_box\":"        << box(alert.person_box) << ","
      << "\"forklift_box\":"      << box(alert.forklift_box) << ","
      << "\"safety_zone_box\":"   << box(alert.safety_zone_box) << ","
      << "\"distance_px\":"       << alert.distance_px
      << "}";
    return o.str();
}

}  // namespace forklift::infrastructure::transport

// ─── Stub fallback (no Boost.Beast) ──────────────────────────────────────────
#else

namespace forklift::infrastructure::transport {

struct WebSocketAlertPublisher::Impl {
    WebSocketServerConfig cfg;
    std::mutex            mu;
    bool                  started{false};
};

WebSocketAlertPublisher::WebSocketAlertPublisher(WebSocketServerConfig cfg)
    : impl_(std::make_unique<Impl>()) {
    impl_->cfg = std::move(cfg);
}

WebSocketAlertPublisher::~WebSocketAlertPublisher() { stop(); }

shared::Result<void> WebSocketAlertPublisher::start() {
    std::lock_guard<std::mutex> lk(impl_->mu);
    if (impl_->started) return {};
    impl_->started = true;
    LOG_WARN("websocket publisher is a log-only stub (build without FSS_WITH_BOOST_BEAST)");
    LOG_INFO("websocket publisher started host=" << impl_->cfg.host
             << " port=" << impl_->cfg.port << " path=" << impl_->cfg.path);
    return {};
}

void WebSocketAlertPublisher::stop() {
    std::lock_guard<std::mutex> lk(impl_->mu);
    if (!impl_->started) return;
    impl_->started = false;
    LOG_INFO("websocket publisher stopped");
}

shared::Result<void> WebSocketAlertPublisher::publish(const domain::Alert& alert) {
    LOG_INFO("ALERT " << serialize(alert));
    return {};
}

std::string WebSocketAlertPublisher::serialize(const domain::Alert& alert) {
    using namespace std::chrono;
    const auto epoch_ms = duration_cast<milliseconds>(
                              alert.timestamp.time_since_epoch()).count();
    auto box = [](const domain::BoundingBox& b) {
        std::ostringstream o;
        o << "{\"x\":" << b.x << ",\"y\":" << b.y
          << ",\"w\":" << b.width << ",\"h\":" << b.height << "}";
        return o.str();
    };
    std::ostringstream o;
    o << "{"
      << "\"schema_version\":1,"
      << "\"alert_id\":\""        << alert.alert_id << "\","
      << "\"type\":\""            << domain::to_string(alert.type) << "\","
      << "\"severity\":\""        << domain::to_string(alert.severity) << "\","
      << "\"camera_id\":\""       << alert.camera_id << "\","
      << "\"timestamp_ms\":"      << epoch_ms << ","
      << "\"person_track_id\":"   << alert.person_track_id << ","
      << "\"forklift_track_id\":" << alert.forklift_track_id << ","
      << "\"person_box\":"        << box(alert.person_box) << ","
      << "\"forklift_box\":"      << box(alert.forklift_box) << ","
      << "\"safety_zone_box\":"   << box(alert.safety_zone_box) << ","
      << "\"distance_px\":"       << alert.distance_px
      << "}";
    return o.str();
}

}  // namespace forklift::infrastructure::transport

#endif  // FSS_WITH_BOOST_BEAST
