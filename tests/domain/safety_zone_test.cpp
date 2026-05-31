#include "../test_harness.h"

#include "forklift/domain/SafetyZone.h"

using forklift::domain::BoundingBox;
using forklift::domain::SafetyZone;

TEST_CASE("SafetyZone.contains_point — interior point") {
    SafetyZone z;
    z.zone_box = {0, 0, 100, 100};
    EXPECT_TRUE(z.contains_point(50, 50));
    EXPECT_TRUE(!z.contains_point(150, 50));
}

TEST_CASE("SafetyZone.overlaps — overlapping box") {
    SafetyZone z;
    z.zone_box = {0, 0, 100, 100};
    BoundingBox person{90, 90, 20, 20};
    EXPECT_TRUE(z.overlaps(person));
}
