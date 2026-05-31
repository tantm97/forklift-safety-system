#include "../test_harness.h"

#include <chrono>

#include "forklift/application/RiskDetectionService.h"
#include "forklift/application/SafetyZoneService.h"

using namespace forklift;

namespace {

domain::Detection make_det(domain::ObjectClass c, float x, float y, float w, float h, int id) {
    domain::Detection d;
    d.cls        = c;
    d.box        = {x, y, w, h};
    d.confidence = 0.9F;
    d.track_id   = id;
    return d;
}

}  // namespace

TEST_CASE("RiskDetectionService — person inside zone triggers exactly one alert") {
    application::SafetyZoneConfig cfg;
    cfg.mode      = application::SafetyZoneConfig::Mode::kAbsolutePixels;
    cfg.pad_x_px  = 50.0F;
    cfg.pad_y_px  = 50.0F;
    application::SafetyZoneService  zsvc(cfg);
    application::RiskDetectionService risk(zsvc);

    std::vector<domain::Detection> dets = {
        make_det(domain::ObjectClass::kForklift, 100, 100, 100, 100, 1),
        make_det(domain::ObjectClass::kPerson,    60,  60,  30,  30, 2),
    };

    auto alerts = risk.evaluate("cam-1", dets, std::chrono::system_clock::now());
    EXPECT_EQ(alerts.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(alerts.front().camera_id, std::string{"cam-1"});
    EXPECT_EQ(alerts.front().forklift_track_id, 1);
    EXPECT_EQ(alerts.front().person_track_id, 2);
}

TEST_CASE("RiskDetectionService — person far from forklift does not alert") {
    application::SafetyZoneConfig cfg;
    cfg.mode     = application::SafetyZoneConfig::Mode::kAbsolutePixels;
    cfg.pad_x_px = 10.0F;
    cfg.pad_y_px = 10.0F;
    application::SafetyZoneService   zsvc(cfg);
    application::RiskDetectionService risk(zsvc);

    std::vector<domain::Detection> dets = {
        make_det(domain::ObjectClass::kForklift, 100, 100, 100, 100, 1),
        make_det(domain::ObjectClass::kPerson,   500, 500,  30,  30, 2),
    };
    auto alerts = risk.evaluate("cam-1", dets, std::chrono::system_clock::now());
    EXPECT_EQ(alerts.size(), static_cast<std::size_t>(0));
}

TEST_CASE("RiskDetectionService — multiple persons + forklifts produce cartesian alerts") {
    application::SafetyZoneConfig cfg;
    cfg.mode     = application::SafetyZoneConfig::Mode::kAbsolutePixels;
    cfg.pad_x_px = 5.0F;
    cfg.pad_y_px = 5.0F;
    application::SafetyZoneService   zsvc(cfg);
    application::RiskDetectionService risk(zsvc);

    std::vector<domain::Detection> dets = {
        make_det(domain::ObjectClass::kForklift,   0,   0, 100, 100, 1),
        make_det(domain::ObjectClass::kForklift, 500, 500, 100, 100, 2),
        make_det(domain::ObjectClass::kPerson,    50,  50,  10,  10, 10),
        make_det(domain::ObjectClass::kPerson,   550, 550,  10,  10, 11),
    };
    auto alerts = risk.evaluate("cam-1", dets, std::chrono::system_clock::now());
    EXPECT_EQ(alerts.size(), static_cast<std::size_t>(2));
}
