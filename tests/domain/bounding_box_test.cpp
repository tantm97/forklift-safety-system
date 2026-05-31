#include "../test_harness.h"

#include "forklift/domain/BoundingBox.h"

using forklift::domain::BoundingBox;

TEST_CASE("BoundingBox.iou — identical boxes → 1.0") {
    BoundingBox a{0, 0, 10, 10};
    EXPECT_NEAR(a.iou(a), 1.0F, 1e-6F);
}

TEST_CASE("BoundingBox.iou — disjoint boxes → 0.0") {
    BoundingBox a{0, 0, 10, 10};
    BoundingBox b{100, 100, 10, 10};
    EXPECT_NEAR(a.iou(b), 0.0F, 1e-6F);
}

TEST_CASE("BoundingBox.expanded — symmetric padding clamped at zero") {
    BoundingBox a{5, 5, 10, 10};
    auto e = a.expanded(20, 20);
    EXPECT_NEAR(e.x, 0.0F, 1e-6F);
    EXPECT_NEAR(e.y, 0.0F, 1e-6F);
    EXPECT_NEAR(e.width,  50.0F, 1e-6F);
    EXPECT_NEAR(e.height, 50.0F, 1e-6F);
}

TEST_CASE("BoundingBox.intersects — edge-touching does not count") {
    BoundingBox a{0, 0, 10, 10};
    BoundingBox b{10, 0, 10, 10};
    EXPECT_TRUE(!a.intersects(b));
}
