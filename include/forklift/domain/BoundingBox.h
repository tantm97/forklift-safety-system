// Axis-aligned bounding box in pixel coordinates. Pure value type — no dependencies on OpenCV.

#ifndef FORKLIFT_DOMAIN_BOUNDING_BOX_H_
#define FORKLIFT_DOMAIN_BOUNDING_BOX_H_

namespace forklift::domain {

struct BoundingBox {
    float x{0.0F};       // top-left x in pixels
    float y{0.0F};       // top-left y in pixels
    float width{0.0F};
    float height{0.0F};

    [[nodiscard]] float area()   const noexcept { return width * height; }
    [[nodiscard]] float right()  const noexcept { return x + width;  }
    [[nodiscard]] float bottom() const noexcept { return y + height; }
    [[nodiscard]] float cx()     const noexcept { return x + width  * 0.5F; }
    [[nodiscard]] float cy()     const noexcept { return y + height * 0.5F; }

    // Returns true when this box and `other` share any pixel.
    [[nodiscard]] bool intersects(const BoundingBox& other) const noexcept;

    // Intersection-over-union — used for NMS and overlap-based risk metrics.
    [[nodiscard]] float iou(const BoundingBox& other) const noexcept;

    // Expand box outward by `pad_x` and `pad_y` pixels on each side. Result is clamped to >= 0.
    [[nodiscard]] BoundingBox expanded(float pad_x, float pad_y) const noexcept;
};

}  // namespace forklift::domain

#endif  // FORKLIFT_DOMAIN_BOUNDING_BOX_H_
