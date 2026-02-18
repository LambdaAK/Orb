/**
 * @file Simulation.cpp
 * @brief Implementation of physics simulation logic.
 */

#include "Simulation.hpp"
#include "Math.hpp"
#include <cmath>
#include <algorithm>

namespace {
    const float MAX_DT = 1.0f / 30.0f;
    const float TINY_SPEED = 0.5f;
    const float MIN_SEPARATION = 1.0e-6f;
    /// Constant pull (px/s²) toward well when within this distance (so gravity is obvious)
    const float GRAVITY_PULL = 400.0f;
    const float GRAVITY_RANGE = 2000.0f;  // apply pull within this distance
}

void Simulation::update(float dt) {
    dt = std::min(dt, MAX_DT);

    // --- 0. Apply gravity from wells to particle velocities ---
    for (Particle& p : particles) {
        for (const GravityWell& well : gravityWells) {
            float dx = well.pos.x - p.pos.x;
            float dy = well.pos.y - p.pos.y;
            float distSq = dx * dx + dy * dy;
            if (distSq < 1.0e-6f) continue;
            float dist = std::sqrt(distSq);
            if (dist > GRAVITY_RANGE) continue;
            float invDist = 1.0f / dist;
            float accelMag = GRAVITY_PULL;  // constant pull so effect is unmissable
            p.vel.x += (dx * invDist) * accelMag * dt;
            p.vel.y += (dy * invDist) * accelMag * dt;
        }
    }

    // --- 1. Integrate positions for all particles ---
    for (Particle& p : particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }

    // --- 2. Particle-particle collisions (elastic, with restitution) ---
    const size_t n = particles.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            Particle& a = particles[i];
            Particle& b = particles[j];
            Vec2 delta = b.pos - a.pos;
            float dist = delta.length();
            float sumR = a.radius + b.radius;

            if (dist >= sumR) continue;  // No overlap

            // Collision normal from a toward b (undefined if dist==0)
            Vec2 n = (dist > MIN_SEPARATION) ? delta.normalized() : Vec2(1.0f, 0.0f);

            // Mass proportional to area (r²) so different sizes behave correctly
            float m1 = a.radius * a.radius;
            float m2 = b.radius * b.radius;
            float totalMass = m1 + m2;

            // Position correction: push apart so they are exactly touching
            float overlap = sumR - dist;
            a.pos -= n * (overlap * (m2 / totalMass));
            b.pos += n * (overlap * (m1 / totalMass));

            // Elastic collision with restitution (1D along normal, then apply to velocity)
            float v1n = dot(a.vel, n);
            float v2n = dot(b.vel, n);
            float impulse = (1.0f + restitution) * (v1n - v2n) / totalMass;
            a.vel.x -= impulse * m2 * n.x;
            a.vel.y -= impulse * m2 * n.y;
            b.vel.x += impulse * m1 * n.x;
            b.vel.y += impulse * m1 * n.y;
        }
    }

    // --- 3. Per-particle: drag, wall collisions, tiny-speed clamp ---
    for (Particle& p : particles) {
        // Apply velocity damping (drag) if enabled
        if (drag > 0.0f) {
            float d = 1.0f - drag * dt;
            p.vel.x *= d;
            p.vel.y *= d;
        }

        float r = p.radius;

        // Wall collision detection and response
        if (p.pos.x - r < 0) {
            p.pos.x = r;
            p.vel.x = std::abs(p.vel.x) * restitution;
        }
        if (p.pos.x + r > worldW) {
            p.pos.x = worldW - r;
            p.vel.x = -std::abs(p.vel.x) * restitution;
        }
        if (p.pos.y - r < 0) {
            p.pos.y = r;
            p.vel.y = std::abs(p.vel.y) * restitution;
        }
        if (p.pos.y + r > worldH) {
            p.pos.y = worldH - r;
            p.vel.y = -std::abs(p.vel.y) * restitution;
        }

        // Stop particles that are moving too slowly (prevents jitter)
        // Skip clamp when any gravity wells exist so gravity can pull stationary particles
        float speedSq = p.vel.x * p.vel.x + p.vel.y * p.vel.y;
        bool skipClamp = !gravityWells.empty();
        if (!skipClamp && speedSq < TINY_SPEED * TINY_SPEED) {
            p.vel.x = 0;
            p.vel.y = 0;
        }
    }
}

void Simulation::clear() {
    particles.clear();
    gravityWells.clear();
}

void Simulation::addGravityWell(float x, float y) {
    gravityWells.emplace_back(Vec2(x, y));
}
