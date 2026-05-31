// Port: AlertPublisher — fan-out of safety alerts to one or more sinks.

#ifndef FORKLIFT_APPLICATION_ALERT_PUBLISHER_H_
#define FORKLIFT_APPLICATION_ALERT_PUBLISHER_H_

#include "forklift/domain/Alert.h"
#include "forklift/shared/Result.h"

namespace forklift::application {

class AlertPublisher {
public:
    virtual ~AlertPublisher() = default;

    virtual shared::Result<void> publish(const domain::Alert& alert) = 0;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_ALERT_PUBLISHER_H_
