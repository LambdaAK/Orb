/**
 * @file Math.hpp
 * @brief Basic math utilities: 2D vectors, colors, and helper functions.
 * 
 * This file provides lightweight math types used throughout the particle simulator.
 * No external math library dependencies - everything is custom-built.
 */

#pragma once

#include <cmath>

/**
 * @struct Vec2
 * @brief 2D vector with x and y components.
 * 
 * Provides basic vector operations: addition, subtraction, scalar multiplication.
 * Used for positions and velocities throughout the simulation.
 */
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    /// Add another vector to this one (in-place)
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    /// Subtract another vector from this one (in-place)
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    /// Add two vectors together
    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    /// Subtract two vectors
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    /// Multiply vector by scalar
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }

    /// Dot product
    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    /// Squared length (avoids sqrt; use for distance comparisons)
    float lengthSq() const { return x * x + y * y; }
    /// Length
    float length() const { return std::sqrt(lengthSq()); }
    /// Unit vector in same direction; returns (0,0) if length is zero
    Vec2 normalized() const {
        float len = length();
        if (len <= 0.0f) return Vec2(0.0f, 0.0f);
        return Vec2(x / len, y / len);
    }
};

/// Dot product of two vectors
inline float dot(const Vec2& a, const Vec2& b) { return a.dot(b); }

/**
 * @struct Color
 * @brief RGBA color with components in range [0.0, 1.0].
 * 
 * Used for particle rendering. All components default to 1.0 (white).
 */
struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};

/**
 * @brief Clamp a value between minimum and maximum bounds.
 * @param v Value to clamp
 * @param lo Minimum allowed value
 * @param hi Maximum allowed value
 * @return Clamped value (guaranteed to be in [lo, hi])
 * 
 * Used to ensure particle spawn positions stay within screen bounds.
 */
inline float clamp(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
