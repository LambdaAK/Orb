/**
 * @file Renderer.hpp
 * @brief OpenGL rendering system for drawing particles and UI elements.
 * 
 * Handles:
 * - OpenGL context creation and management
 * - Shader compilation and management
 * - Drawing particles as circles
 * - Drawing drag preview line
 */

#pragma once

#include "Math.hpp"
#include "Particle.hpp"
#include "GravityWell.hpp"
#include <vector>

struct SDL_Window;

/**
 * @class Renderer
 * @brief Manages all OpenGL rendering operations.
 * 
 * Uses OpenGL 3.2 Core Profile with simple shaders to draw particles
 * as filled circles. Handles viewport setup and coordinate transformation.
 */
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    /**
     * @brief Initialize OpenGL context and shaders.
     * @param window SDL window to create OpenGL context for
     * @param width Initial viewport width
     * @param height Initial viewport height
     * @return true if initialization succeeded, false on error
     */
    bool init(SDL_Window* window, int width, int height);
    
    /**
     * @brief Update viewport size when window is resized.
     * @param width New viewport width
     * @param height New viewport height
     */
    void resize(int width, int height);

    /**
     * @brief Clear the screen with dark background color.
     */
    void clear();
    
    /**
     * @brief Draw all particles as circles.
     * @param particles Vector of particles to render
     */
    void drawParticles(const std::vector<Particle>& particles);
    void drawParticleTrails(const std::vector<Particle>& particles);
    void drawGravityWells(const std::vector<GravityWell>& wells);

    /**
     * @brief Draw a line preview for drag-to-spawn interaction.
     * @param from Start position (where mouse was pressed)
     * @param to End position (current mouse position)
     */
    void drawDragPreview(Vec2 from, Vec2 to);

private:
    /**
     * @brief Compile and link shaders, store program ID.
     */
    void initShaders();
    
    /**
     * @brief Draw a single particle as a filled circle using triangle fan.
     * @param p Particle to draw
     */
    void drawCircle(const Particle& p);

    SDL_Window* window_ = nullptr;      ///< SDL window handle
    void* glContext_ = nullptr;          ///< OpenGL context handle
    int width_ = 0;                      ///< Current viewport width
    int height_ = 0;                     ///< Current viewport height
    unsigned int program_ = 0;           ///< Compiled shader program ID
};
