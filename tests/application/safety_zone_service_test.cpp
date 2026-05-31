#include "../test_harness.h"

#include "forklift/application/SafetyZoneService.h"

using namespace forklift;

TEST_CASE("SafetyZoneService — relative mode grows by ratio on each side") {
    application::SafetyZoneConfig cfg;
    cfg.mode      = application::SafetyZoneConfig::Mode::kRelativeToBox;
    cfg.pad_ratio = 0.5F;
    application::SafetyZoneService svc(cfg);

    domain::Detection d;
    d.cls      = domain::ObjectClass::kForklift;
    d.box      = {100, 100, 200, 100};
    d.track_id = 7;

    auto z = svc.zone_for_forklift(d);
    EXPECT_EQ(z.forklift_track_id, 7);
    EXPECT_NEAR(z.zone_box.width,  400.0F, 1e-3F);   // 200 + 2 * 100
    EXPECT_NEAR(z.zone_box.height, 200.0F, 1e-3F);   // 100 + 2 * 50
}

TEST_CASE("SafetyZoneService — absolute mode adds fixed pixels per side") {
    application::SafetyZoneConfig cfg;
    cfg.mode     = application::SafetyZoneConfig::Mode::kAbsolutePixels;
    cfg.pad_x_px = 25.0F;
    cfg.pad_y_px = 50.0F;
    application::SafetyZoneService svc(cfg);

    domain::Detection d;
    d.cls = domain::ObjectClass::kForklift;
    d.box = {100, 100, 200, 100};

    auto z = svc.zone_for_forklift(d);
    EXPECT_NEAR(z.zone_box.width,  250.0F, 1e-3F);   // 200 + 2 * 25
    EXPECT_NEAR(z.zone_box.height, 200.0F, 1e-3F);   // 100 + 2 * 50
}
