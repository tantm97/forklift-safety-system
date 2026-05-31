#include "forklift/domain/BoundingBox.h"

#include <algorithm>

namespace forklift::domain {

bool BoundingBox::intersects(const BoundingBox& other) const noexcept {
    return !(other.x >= right()  ||
             other.right() <= x  ||
             other.y >= bottom() ||
             other.bottom() <= y);
}

float BoundingBox::iou(const BoundingBox& other) const noexcept {
    const float ix1 = std::max(x, other.x);
    const float iy1 = std::max(y, other.y);
    const float ix2 = std::min(right(),  other.right());
    const float iy2 = std::min(bottom(), other.bottom());

    const float iw = std::max(0.0F, ix2 - ix1);
    const float ih = std::max(0.0F, iy2 - iy1);
    const float intersection = iw * ih;

    const float union_area = area() + other.area() - intersection;
    if (union_area <= 0.0F) return 0.0F;
    return intersection / union_area;
}

BoundingBox BoundingBox::expanded(float pad_x, float pad_y) const noexcept {
    BoundingBox out;
    out.x      = std::max(0.0F, x - pad_x);
    out.y      = std::max(0.0F, y - pad_y);
    out.width  = std::max(0.0F, width  + 2.0F * pad_x);
    out.height = std::max(0.0F, height + 2.0F * pad_y);
    return out;
}

}  // namespace forklift::domain
