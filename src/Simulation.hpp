/**
 * @file Simulation.hpp
 * @brief Physics simulation logic: particle updates and wall collisions.
 * 
 * Handles all physics calculations including:
 * - Position integration (movement)
 * - Velocity damping (optional drag)
 * - Particle-particle elastic collisions
 * - Wall collision detection and response
 */

#pragma once

#include <vector>
#include "Particle.hpp"
#include "GravityWell.hpp"

/**
 * @struct Simulation
 * @brief Manages the particle physics simulation.
 * 
 * Contains particles, gravity wells, and simulation parameters. update() does:
 * gravity forces, integrate, particle-particle collisions, drag, walls.
 */
struct Simulation {
    float worldW = 1280.0f;
    float worldH = 720.0f;
    float restitution = 0.9f;
    float drag = 0.0f;

    std::vector<Particle> particles;
    std::vector<GravityWell> gravityWells;

    void update(float dt);
    void clear();
    void addGravityWell(float x, float y);
};
