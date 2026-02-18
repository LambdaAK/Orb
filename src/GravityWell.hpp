#pragma once

#include "Math.hpp"

/** Attractor that exerts inverse-square gravitational pull on particles. */
struct GravityWell {
    Vec2 pos;
    float strength = 1200000.0f; ///< Acceleration = strength / r² (very strong so pull is obvious)
    float minRadius = 8.0f;     ///< Clamp distance below this to avoid singularity

    GravityWell() = default;
    GravityWell(Vec2 pos_, float strength_ = 1200000.0f, float minRadius_ = 8.0f)
        : pos(pos_), strength(strength_), minRadius(minRadius_) {}
};
