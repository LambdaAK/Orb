/**
 * @file App.cpp
 * @brief Main application implementation.
 */

#include "App.hpp"
#include "Renderer.hpp"
#include "Simulation.hpp"
#include "Math.hpp"
#include "Particle.hpp"
#include <SDL2/SDL.h>
#include <cstdlib>
#include <cmath>
#include <cstdio>

namespace {

/**
 * @brief Generate a random float in range [0.0, 1.0].
 */
float randFloat() {
    return (float)std::rand() / (float)RAND_MAX;
}

/**
 * @brief Generate a random bright color using HSV color space.
 * @return Color with high saturation and value for visibility
 * 
 * Converts HSV to RGB:
 * - Hue: random (0-1)
 * - Saturation: 0.2-1.0 (ensures colorfulness)
 * - Value: 0.85-1.0 (ensures brightness)
 */
Color randomBrightColor() {
    float h = randFloat();
    float s = 0.2f + 0.8f * randFloat();
    float v = 0.85f + 0.15f * randFloat();
    float r, g, b;
    float c = v * s;
    float x = c * (1.0f - std::fabs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    if (h < 1.0f/6.0f)      { r = c; g = x; b = 0; }
    else if (h < 2.0f/6.0f) { r = x; g = c; b = 0; }
    else if (h < 3.0f/6.0f) { r = 0; g = c; b = x; }
    else if (h < 4.0f/6.0f) { r = 0; g = x; b = c; }
    else if (h < 5.0f/6.0f) { r = x; g = 0; b = c; }
    else                    { r = c; g = 0; b = x; }
    return Color(r + m, g + m, b + m, 1.0f);
}

} // namespace

bool App::init() {
    // Initialize SDL2 video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    std::srand(12345);  // Fixed seed for deterministic random colors

    // Create window with OpenGL support
    window = SDL_CreateWindow("Particle Sandbox",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Initialize OpenGL renderer
    renderer = new Renderer();
    if (!renderer->init(window, width, height)) {
        delete renderer;
        renderer = nullptr;
        return false;
    }

    // Initialize physics simulation
    simulation = new Simulation();
    simulation->worldW = (float)width;
    simulation->worldH = (float)height;

    return true;
}

void App::shutdown() {
    delete simulation;
    simulation = nullptr;
    delete renderer;
    renderer = nullptr;
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void App::spawnParticle(float x, float y, float vx, float vy) {
    float r = particleRadius;
    float W = simulation->worldW;
    float H = simulation->worldH;
    
    // Clamp spawn position to ensure particle starts within bounds
    // Account for radius so particle doesn't spawn partially off-screen
    x = clamp(x, r, W - r);
    y = clamp(y, r, H - r);
    
    Vec2 pos(x, y);
    Vec2 vel(vx, vy);
    Color color = randomBrightColor();
    simulation->particles.emplace_back(pos, vel, r, color);
}

void App::handleEvent(void* eventPtr) {
    SDL_Event& e = *static_cast<SDL_Event*>(eventPtr);
    switch (e.type) {
        case SDL_QUIT:
            // Window close button clicked
            running = false;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE)
                running = false;  // Exit on Esc
            else if (e.key.keysym.sym == SDLK_r)
                simulation->clear();  // Clear all particles
            else if (e.key.keysym.sym == SDLK_SPACE)
                paused = !paused;  // Toggle pause
            break;
        case SDL_MOUSEBUTTONDOWN:
            // Start drag: record mouse position
            if (e.button.button == SDL_BUTTON_LEFT) {
                dragActive = true;
                dragStartX = (float)e.button.x;
                dragStartY = (float)e.button.y;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            // End drag: spawn particle with velocity based on drag distance
            if (e.button.button == SDL_BUTTON_LEFT && dragActive) {
                float endX = (float)e.button.x;
                float endY = (float)e.button.y;
                // Velocity = drag vector * strength multiplier
                float vx = (endX - dragStartX) * velocityStrength;
                float vy = (endY - dragStartY) * velocityStrength;
                spawnParticle(dragStartX, dragStartY, vx, vy);
                dragActive = false;
            }
            break;
        case SDL_WINDOWEVENT:
            // Handle window resize: update world bounds and viewport
            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                width = e.window.data1;
                height = e.window.data2;
                simulation->worldW = (float)width;
                simulation->worldH = (float)height;
                renderer->resize(width, height);
            }
            break;
        default:
            break;
    }
}

void App::update(float dt) {
    if (!paused)
        simulation->update(dt);
}

void App::render() {
    renderer->clear();
    renderer->drawParticles(simulation->particles);
    if (dragActive) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        renderer->drawDragPreview(Vec2(dragStartX, dragStartY), Vec2((float)mx, (float)my));
    }
    SDL_GL_SwapWindow(window);
}

void App::run() {
    // High-resolution timer for accurate delta-time calculation
    Uint64 lastTicks = SDL_GetPerformanceCounter();
    
    while (running) {
        // Process all pending events
        SDL_Event e;
        while (SDL_PollEvent(&e))
            handleEvent(&e);

        // Calculate delta time in seconds using high-resolution counter
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - lastTicks) / (float)SDL_GetPerformanceFrequency();
        lastTicks = now;

        // Update simulation and render frame
        update(dt);
        render();
    }
}
