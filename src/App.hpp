/**
 * @file App.hpp
 * @brief Main application class: window management, event handling, game loop.
 * 
 * Coordinates SDL2 window, OpenGL renderer, and physics simulation.
 * Handles user input (mouse, keyboard) and manages the main game loop.
 */

#pragma once

struct SDL_Window;
struct SDL_Renderer;
struct TTF_Font;
class Renderer;
struct Simulation;

/** Placeable item types (selected from the menu) */
enum class PlaceableType { Particle };

/**
 * @struct App
 * @brief Main application controller.
 * 
 * Manages the entire application lifecycle:
 * - Window creation and event handling
 * - Input processing (spawn particles, pause, clear)
 * - Game loop with delta-time calculation
 * - Coordination between renderer and simulation
 */
struct App {
    SDL_Window* window = nullptr;        ///< Main sandbox window
    SDL_Window* menuWindow = nullptr;    ///< Place/tools menu window
    SDL_Renderer* menuRenderer = nullptr;///< 2D renderer for menu (no OpenGL)
    TTF_Font* menuFont = nullptr;        ///< Font for menu labels
    Renderer* renderer = nullptr;       ///< OpenGL renderer instance
    Simulation* simulation = nullptr;   ///< Physics simulation instance

    int width = 1280;                   ///< Main window width in pixels
    int height = 720;                   ///< Main window height in pixels

    PlaceableType selectedPlaceable = PlaceableType::Particle;  ///< Current tool from menu

    bool running = true;                ///< Main loop flag (false = exit)
    bool paused = false;                ///< Simulation pause state

    // Click-drag spawn state
    bool dragActive = false;            ///< True while user is dragging to spawn
    float dragStartX = 0.0f;            ///< Mouse X position when drag started
    float dragStartY = 0.0f;           ///< Mouse Y position when drag started
    float velocityStrength = 6.0f;     ///< Multiplier for drag-to-velocity conversion
    float particleRadius = 3.5f;        ///< Default radius for spawned particles (smaller, modern look)

    /**
     * @brief Initialize SDL, create window, set up renderer and simulation.
     * @return true on success, false on error
     */
    bool init();
    
    /**
     * @brief Clean up all resources and shutdown SDL.
     */
    void shutdown();
    
    /**
     * @brief Run the main game loop until exit.
     * 
     * Processes events, updates simulation, renders frame, calculates delta time.
     */
    void run();

    /**
     * @brief Process a single SDL event.
     * @param eventPtr Pointer to SDL_Event structure
     * 
     * Handles: quit, keyboard (Esc, R, Space), mouse (click-drag spawn), window resize.
     */
    void handleEvent(void* event);
    
    /**
     * @brief Update simulation by one frame.
     * @param dt Time delta in seconds
     */
    void update(float dt);
    
    /**
     * @brief Render one frame: main window (particles, drag preview) and menu window.
     */
    void render();
    /** @brief Draw the place/tools menu in the menu window */
    void renderMenu();
    
    /**
     * @brief Spawn a new particle at the given position with given velocity.
     * @param x Spawn X position (will be clamped to bounds)
     * @param y Spawn Y position (will be clamped to bounds)
     * @param vx Initial X velocity
     * @param vy Initial Y velocity
     * 
     * Position is clamped to ensure particle starts within screen bounds.
     * Particle gets a random bright color.
     */
    void spawnParticle(float x, float y, float vx, float vy);
};
