/**
 * @file main.cpp
 * @brief Application entry point.
 * 
 * Creates the App instance, initializes it, runs the main loop, and cleans up.
 */

#include "App.hpp"
#include <cstdio>

/**
 * @brief Main entry point.
 * @param argc Command-line argument count (unused)
 * @param argv Command-line arguments (unused)
 * @return 0 on success, 1 on initialization failure
 */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    App app;
    if (!app.init()) {
        std::fprintf(stderr, "App init failed.\n");
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
