// Copyright (c) 2026 forklift-safety-system contributors.
// SPDX-License-Identifier: Apache-2.0
//
// Domain enum: ObjectClass — the closed set of object categories the system reasons about.
// Adding a new class requires a domain-level decision (ADR), not just a model retrain.

#ifndef FORKLIFT_DOMAIN_OBJECT_CLASS_H_
#define FORKLIFT_DOMAIN_OBJECT_CLASS_H_

#include <cstdint>
#include <string_view>

namespace forklift::domain {

enum class ObjectClass : std::uint8_t {
    kUnknown  = 0,
    kPerson   = 1,
    kForklift = 2,
};

constexpr std::string_view to_string(ObjectClass cls) noexcept {
    switch (cls) {
        case ObjectClass::kPerson:   return "person";
        case ObjectClass::kForklift: return "forklift";
        case ObjectClass::kUnknown:
        default:                     return "unknown";
    }
}

}  // namespace forklift::domain

#endif  // FORKLIFT_DOMAIN_OBJECT_CLASS_H_
