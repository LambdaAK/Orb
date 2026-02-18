/**
 * @file Particle.hpp
 * @brief Particle data structure definition.
 * 
 * A particle represents a single moving circle in the simulation.
 * Contains position, velocity, size (radius), and visual color.
 */

#pragma once

#include "Math.hpp"

/**
 * @struct Particle
 * @brief Represents a single particle in the simulation.
 * 
 * Each particle has:
 * - Position (Vec2): current x,y location in world space
 * - Velocity (Vec2): current x,y velocity in pixels per second
 * - Radius (float): size of the particle circle
 * - Color: RGBA color for rendering
 */
struct Particle {
    Vec2 pos;
    Vec2 vel;
    float radius;
    Color color;
    
    // Trail history for rendering glowing trails
    static constexpr int MAX_TRAIL_LENGTH = 60;
    Vec2 trail[MAX_TRAIL_LENGTH];
    int trailLength = 0;
    int trailIndex = 0;

    Particle(Vec2 pos_, Vec2 vel_, float radius_, Color color_)
        : pos(pos_), vel(vel_), radius(radius_), color(color_) {
        // Initialize trail with current position
        for (int i = 0; i < MAX_TRAIL_LENGTH; ++i) trail[i] = pos_;
        trailLength = 1;
    }
    
    void updateTrail() {
        trail[trailIndex] = pos;
        trailIndex = (trailIndex + 1) % MAX_TRAIL_LENGTH;
        if (trailLength < MAX_TRAIL_LENGTH) trailLength++;
    }
};
