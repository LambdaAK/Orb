/**
 * @file Simulation.cpp
 * @brief Implementation of physics simulation logic.
 */

#include "Simulation.hpp"
#include "Math.hpp"
#include <cmath>
#include <algorithm>

namespace {
    /// Maximum allowed time step (30 FPS minimum) - prevents explosion on frame drops
    const float MAX_DT = 1.0f / 30.0f;
    /// Speed threshold below which particles are stopped (prevents jitter)
    const float TINY_SPEED = 0.5f;
}

void Simulation::update(float dt) {
    // Clamp delta time to prevent huge jumps on frame drops (e.g. alt-tab)
    dt = std::min(dt, MAX_DT);

    for (Particle& p : particles) {
        // Integrate position: move particle based on velocity
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;

        // Apply velocity damping (drag) if enabled
        if (drag > 0.0f) {
            float d = 1.0f - drag * dt;
            p.vel.x *= d;
            p.vel.y *= d;
        }

        float r = p.radius;

        // Wall collision detection and response
        // Left wall: particle's left edge hits x=0
        if (p.pos.x - r < 0) {
            p.pos.x = r;  // Reposition to be exactly touching wall
            p.vel.x = std::abs(p.vel.x) * restitution;  // Bounce right with energy loss
        }
        // Right wall: particle's right edge hits worldW
        if (p.pos.x + r > worldW) {
            p.pos.x = worldW - r;
            p.vel.x = -std::abs(p.vel.x) * restitution;  // Bounce left
        }
        // Top wall: particle's top edge hits y=0
        if (p.pos.y - r < 0) {
            p.pos.y = r;
            p.vel.y = std::abs(p.vel.y) * restitution;  // Bounce down
        }
        // Bottom wall: particle's bottom edge hits worldH
        if (p.pos.y + r > worldH) {
            p.pos.y = worldH - r;
            p.vel.y = -std::abs(p.vel.y) * restitution;  // Bounce up
        }

        // Stop particles that are moving too slowly (prevents jitter)
        float speedSq = p.vel.x * p.vel.x + p.vel.y * p.vel.y;
        if (speedSq < TINY_SPEED * TINY_SPEED) {
            p.vel.x = 0;
            p.vel.y = 0;
        }
    }
}

void Simulation::clear() {
    particles.clear();
}
