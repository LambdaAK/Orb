/**
 * @file Simulation.hpp
 * @brief Physics simulation logic: particle updates and wall collisions.
 * 
 * Handles all physics calculations including:
 * - Position integration (movement)
 * - Velocity damping (optional drag)
 * - Wall collision detection and response
 */

#pragma once

#include <vector>
#include "Particle.hpp"

/**
 * @struct Simulation
 * @brief Manages the particle physics simulation.
 * 
 * Contains all particles and simulation parameters. The update() method
 * advances the simulation by one time step, handling movement and collisions.
 */
struct Simulation {
    float worldW = 1280.0f;      ///< World width in pixels
    float worldH = 720.0f;        ///< World height in pixels
    float restitution = 0.9f;     ///< Bounce coefficient [0-1]: 1.0 = perfect bounce, 0.0 = no bounce
    float drag = 0.0f;            ///< Velocity damping per second [0-1]: 0 = no drag, 1 = instant stop

    std::vector<Particle> particles;  ///< All particles in the simulation

    /**
     * @brief Advance simulation by one time step.
     * @param dt Time delta in seconds (will be clamped to prevent large jumps)
     * 
     * Updates all particle positions, applies drag, handles wall collisions,
     * and stops particles that are moving too slowly to prevent jitter.
     */
    void update(float dt);
    
    /**
     * @brief Remove all particles from the simulation.
     */
    void clear();
};
