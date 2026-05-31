// WebSocketAlertPublisher — broadcasts JSON alert events to all connected clients.
// Implementation lives behind a PIMPL so we can swap the underlying WS library
// (e.g. websocketpp, uWebSockets, Boost.Beast) without touching call sites.

#ifndef FORKLIFT_INFRASTRUCTURE_TRANSPORT_WEBSOCKET_ALERT_PUBLISHER_H_
#define FORKLIFT_INFRASTRUCTURE_TRANSPORT_WEBSOCKET_ALERT_PUBLISHER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "forklift/application/AlertPublisher.h"

namespace forklift::infrastructure::transport {

struct WebSocketServerConfig {
    std::string host{"0.0.0.0"};
    std::uint16_t port{8765};
    std::string path{"/ws/alerts"};
};

class WebSocketAlertPublisher final : public application::AlertPublisher {
public:
    explicit WebSocketAlertPublisher(WebSocketServerConfig cfg);
    ~WebSocketAlertPublisher() override;

    shared::Result<void> start();
    void                 stop();

    shared::Result<void> publish(const domain::Alert& alert) override;

    // Exposed for tests / introspection.
    static std::string serialize(const domain::Alert& alert);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace forklift::infrastructure::transport

#endif  // FORKLIFT_INFRASTRUCTURE_TRANSPORT_WEBSOCKET_ALERT_PUBLISHER_H_
