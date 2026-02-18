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
#include <SDL2/SDL_ttf.h>
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
    float s = 0.6f + 0.4f * randFloat();  // Higher saturation for vibrant colors
    float v = 0.9f + 0.1f * randFloat();   // Brighter values for modern glow look
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
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return false;
    }
    std::srand(12345);  // Fixed seed for deterministic random colors

    // Create main window with OpenGL support
    window = SDL_CreateWindow("Particle Sandbox",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width, height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Create menu window (to the left of main window)
    int mainX = 0, mainY = 0;
    SDL_GetWindowPosition(window, &mainX, &mainY);
    const int menuW = 200;
    const int menuH = 176;  // Two slots: Particle, Gravity Well
    menuWindow = SDL_CreateWindow("Place",
                                  mainX - menuW - 20, mainY,
                                  menuW, menuH,
                                  SDL_WINDOW_SHOWN);
    if (!menuWindow) {
        std::fprintf(stderr, "SDL_CreateWindow (menu) failed: %s\n", SDL_GetError());
        return false;
    }
    menuRenderer = SDL_CreateRenderer(menuWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!menuRenderer) {
        std::fprintf(stderr, "SDL_CreateRenderer (menu) failed: %s\n", SDL_GetError());
        return false;
    }

    // Load font for menu labels (try system fonts on macOS)
    #ifdef __APPLE__
    menuFont = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 16);
    if (!menuFont) {
        menuFont = TTF_OpenFont("/Library/Fonts/Arial.ttf", 16);
    }
    #else
    menuFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!menuFont) {
        menuFont = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 16);
    }
    #endif
    if (!menuFont) {
        std::fprintf(stderr, "Warning: Could not load font, labels will not render: %s\n", TTF_GetError());
    }

    // Initialize OpenGL renderer for main window
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
    if (menuFont) {
        TTF_CloseFont(menuFont);
        menuFont = nullptr;
    }
    if (menuRenderer) {
        SDL_DestroyRenderer(menuRenderer);
        menuRenderer = nullptr;
    }
    if (menuWindow) {
        SDL_DestroyWindow(menuWindow);
        menuWindow = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    TTF_Quit();
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

void App::spawnGravityWell(float x, float y) {
    simulation->addGravityWell(x, y);
}

void App::handleEvent(void* eventPtr) {
    SDL_Event& e = *static_cast<SDL_Event*>(eventPtr);
    Uint32 mainID = SDL_GetWindowID(window);
    Uint32 menuID = menuWindow ? SDL_GetWindowID(menuWindow) : 0;

    switch (e.type) {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            else if (e.key.keysym.sym == SDLK_r)
                simulation->clear();
            else if (e.key.keysym.sym == SDLK_SPACE)
                paused = !paused;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (e.button.button != SDL_BUTTON_LEFT) break;
            if (e.button.windowID == menuID) {
                int mx = e.button.x, my = e.button.y;
                if (mx >= 12 && mx < 188 && my >= 36 && my < 96)
                    selectedPlaceable = PlaceableType::Particle;
                else if (mx >= 12 && mx < 188 && my >= 104 && my < 164)
                    selectedPlaceable = PlaceableType::GravityWell;
            } else if (e.button.windowID == mainID) {
                dragActive = true;
                dragStartX = (float)e.button.x;
                dragStartY = (float)e.button.y;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e.button.button == SDL_BUTTON_LEFT && dragActive && e.button.windowID == mainID) {
                float endX = (float)e.button.x;
                float endY = (float)e.button.y;
                float vx = (endX - dragStartX) * velocityStrength;
                float vy = (endY - dragStartY) * velocityStrength;
                if (selectedPlaceable == PlaceableType::Particle)
                    spawnParticle(dragStartX, dragStartY, vx, vy);
                else if (selectedPlaceable == PlaceableType::GravityWell)
                    spawnGravityWell(dragStartX, dragStartY);
                dragActive = false;
            } else if (e.button.button == SDL_BUTTON_LEFT && dragActive) {
                dragActive = false;
            }
            break;
        case SDL_WINDOWEVENT:
            if (e.window.windowID == mainID && e.window.event == SDL_WINDOWEVENT_RESIZED) {
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
    renderer->drawGravityWells(simulation->gravityWells);
    renderer->drawParticles(simulation->particles);
    if (dragActive) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        renderer->drawDragPreview(Vec2(dragStartX, dragStartY), Vec2((float)mx, (float)my));
    }
    SDL_GL_SwapWindow(window);

    renderMenu();
}

void App::renderMenu() {
    if (!menuRenderer) return;
    const int menuW = 200;

    SDL_SetRenderDrawColor(menuRenderer, 28, 28, 36, 255);
    SDL_RenderClear(menuRenderer);

    // Title bar
    SDL_Rect titleRect = { 0, 0, menuW, 28 };
    SDL_SetRenderDrawColor(menuRenderer, 45, 45, 58, 255);
    SDL_RenderFillRect(menuRenderer, &titleRect);

    // Render "Place" title text
    if (menuFont) {
        SDL_Color white = { 255, 255, 255, 255 };
        SDL_Surface* textSurf = TTF_RenderText_Solid(menuFont, "Place", white);
        if (textSurf) {
            SDL_Texture* textTex = SDL_CreateTextureFromSurface(menuRenderer, textSurf);
            if (textTex) {
                SDL_Rect textRect = { 8, 6, textSurf->w, textSurf->h };
                SDL_RenderCopy(menuRenderer, textTex, nullptr, &textRect);
                SDL_DestroyTexture(textTex);
            }
            SDL_FreeSurface(textSurf);
        }
    }

    // Particle slot
    SDL_Rect slotRect = { 12, 36, 176, 60 };
    bool particleSelected = (selectedPlaceable == PlaceableType::Particle);
    SDL_SetRenderDrawColor(menuRenderer, particleSelected ? 70 : 50, particleSelected ? 70 : 50, particleSelected ? 90 : 65, 255);
    SDL_RenderFillRect(menuRenderer, &slotRect);
    SDL_SetRenderDrawColor(menuRenderer, particleSelected ? 255 : 100, particleSelected ? 255 : 100, particleSelected ? 255 : 120, 255);
    SDL_RenderDrawRect(menuRenderer, &slotRect);
    SDL_Rect iconRect = { 12 + 20, 36 + 12, 12, 12 };
    SDL_SetRenderDrawColor(menuRenderer, 180, 180, 255, 255);
    SDL_RenderFillRect(menuRenderer, &iconRect);
    if (menuFont) {
        SDL_Color c = { static_cast<Uint8>(particleSelected ? 255 : 200), static_cast<Uint8>(particleSelected ? 255 : 200), static_cast<Uint8>(particleSelected ? 255 : 220), 255 };
        SDL_Surface* surf = TTF_RenderText_Solid(menuFont, "Particle", c);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(menuRenderer, surf);
            if (tex) { SDL_Rect r = { 12 + 40, 36 + 20, surf->w, surf->h }; SDL_RenderCopy(menuRenderer, tex, nullptr, &r); SDL_DestroyTexture(tex); }
            SDL_FreeSurface(surf);
        }
    }

    // Gravity Well slot
    SDL_Rect wellSlotRect = { 12, 104, 176, 60 };
    bool wellSelected = (selectedPlaceable == PlaceableType::GravityWell);
    SDL_SetRenderDrawColor(menuRenderer, wellSelected ? 70 : 50, wellSelected ? 50 : 40, wellSelected ? 90 : 65, 255);
    SDL_RenderFillRect(menuRenderer, &wellSlotRect);
    SDL_SetRenderDrawColor(menuRenderer, wellSelected ? 255 : 100, wellSelected ? 100 : 80, wellSelected ? 255 : 120, 255);
    SDL_RenderDrawRect(menuRenderer, &wellSlotRect);
    SDL_Rect wellIconRect = { 12 + 20, 104 + 12, 12, 12 };
    SDL_SetRenderDrawColor(menuRenderer, 140, 80, 200, 255);
    SDL_RenderFillRect(menuRenderer, &wellIconRect);
    if (menuFont) {
        SDL_Color c = { static_cast<Uint8>(wellSelected ? 255 : 200), static_cast<Uint8>(wellSelected ? 200 : 180), static_cast<Uint8>(wellSelected ? 255 : 220), 255 };
        SDL_Surface* surf = TTF_RenderText_Solid(menuFont, "Gravity Well", c);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(menuRenderer, surf);
            if (tex) { SDL_Rect r = { 12 + 40, 104 + 20, surf->w, surf->h }; SDL_RenderCopy(menuRenderer, tex, nullptr, &r); SDL_DestroyTexture(tex); }
            SDL_FreeSurface(surf);
        }
    }

    SDL_RenderPresent(menuRenderer);
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
