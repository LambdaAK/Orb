/**
 * @file Renderer.cpp
 * @brief OpenGL rendering implementation.
 */

#include "Renderer.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION 1  // Silence macOS OpenGL deprecation warnings
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace {

/**
 * Vertex shader source code.
 * Transforms 2D positions from world space to clip space.
 * Passes color from vertex to fragment shader.
 */
const char* VERT_SRC = R"(
#version 150
in vec2 aPos;
in vec4 aColor;
out vec4 vColor;
uniform mat4 uProj;
void main() {
    gl_Position = uProj * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

/**
 * Fragment shader source code.
 * Simply outputs the interpolated vertex color.
 */
const char* FRAG_SRC = R"(
#version 150
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)";

/**
 * @brief Compile a GLSL shader from source code.
 * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
 * @param src Shader source code string
 * @return Shader ID on success, 0 on failure (prints error to stderr)
 */
unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(id, sizeof(buf), nullptr, buf);
        std::fprintf(stderr, "Shader compile error: %s\n", buf);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

/**
 * @brief Create and link a complete shader program from vertex and fragment shaders.
 * @return Program ID on success, 0 on failure (prints error to stderr)
 */
unsigned int createProgram() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    if (!vs || !fs) return 0;
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        std::fprintf(stderr, "Program link error: %s\n", buf);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

} // namespace

Renderer::~Renderer() {
    if (glContext_ && window_) {
        SDL_GL_MakeCurrent(window_, nullptr);
        SDL_GL_DeleteContext(static_cast<SDL_GLContext>(glContext_));
    }
}

bool Renderer::init(SDL_Window* window, int width, int height) {
    window_ = window;
    width_ = width;
    height_ = height;

    // Request OpenGL 3.2 Core Profile (required on macOS)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // Enable double buffering

    SDL_GLContext ctx = SDL_GL_CreateContext(window_);
    if (!ctx) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }
    glContext_ = ctx;

    if (SDL_GL_MakeCurrent(window_, ctx) != 0) {
        std::fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        return false;
    }

    initShaders();
    if (!program_) return false;

    // Enable alpha blending for glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for modern glow effect
    resize(width_, height_);
    return true;
}

void Renderer::initShaders() {
    program_ = createProgram();
}

void Renderer::resize(int width, int height) {
    width_ = width;
    height_ = height;
    glViewport(0, 0, width_, height_);
}

void Renderer::clear() {
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);  // Darker background for better glow contrast
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawParticles(const std::vector<Particle>& particles) {
    glUseProgram(program_);
    for (const Particle& p : particles) {
        drawCircle(p);
    }
}

void Renderer::drawGravityWells(const std::vector<GravityWell>& wells) {
    const int SEGMENTS = 32;
    const float drawRadius = 24.0f;  // Visual radius for the well
    float proj[16] = {
        2.0f / (float)width_, 0, 0, 0,
        0, -2.0f / (float)height_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUseProgram(program_);
    glUniformMatrix4fv(glGetUniformLocation(program_, "uProj"), 1, GL_FALSE, proj);

    for (const GravityWell& well : wells) {
        float verts[(SEGMENTS + 2) * 6];
        int i = 0;
        // Center - dark
        verts[i++] = well.pos.x;
        verts[i++] = well.pos.y;
        verts[i++] = 0.15f;
        verts[i++] = 0.08f;
        verts[i++] = 0.25f;
        verts[i++] = 0.9f;
        for (int s = 0; s <= SEGMENTS; ++s) {
            float a = (float)s / (float)SEGMENTS * 6.283185307f;
            verts[i++] = well.pos.x + drawRadius * std::cos(a);
            verts[i++] = well.pos.y + drawRadius * std::sin(a);
            verts[i++] = 0.35f;
            verts[i++] = 0.2f;
            verts[i++] = 0.5f;
            verts[i++] = 0.5f;  // Slightly transparent ring
        }
        unsigned int vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * i), verts, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
}

void Renderer::drawCircle(const Particle& p) {
    const int SEGMENTS = 32;  // Number of segments to approximate circle
    
    // Set up projection matrix once (shared for glow and core)
    float W = (float)width_;
    float H = (float)height_;
    float proj[16] = {
        2.0f/W, 0, 0, 0,
        0, -2.0f/H, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    int projLoc = glGetUniformLocation(program_, "uProj");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, proj);

    // --- Render outer glow (larger, semi-transparent) ---
    float glowRadius = p.radius * 2.5f;  // Glow extends 2.5x the core radius
    float glowAlpha = 0.3f;  // Glow transparency
    
    float glowVerts[(SEGMENTS + 2) * 6];
    int i = 0;
    glowVerts[i++] = p.pos.x;
    glowVerts[i++] = p.pos.y;
    glowVerts[i++] = p.color.r;
    glowVerts[i++] = p.color.g;
    glowVerts[i++] = p.color.b;
    glowVerts[i++] = glowAlpha;
    
    for (int s = 0; s <= SEGMENTS; ++s) {
        float a = (float)s / (float)SEGMENTS * 6.283185307f;
        glowVerts[i++] = p.pos.x + glowRadius * std::cos(a);
        glowVerts[i++] = p.pos.y + glowRadius * std::sin(a);
        glowVerts[i++] = p.color.r;
        glowVerts[i++] = p.color.g;
        glowVerts[i++] = p.color.b;
        glowVerts[i++] = 0.0f;  // Fade to transparent at edge
    }

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * i), glowVerts, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    // --- Render bright core (smaller, fully opaque) ---
    float coreVerts[(SEGMENTS + 2) * 6];
    i = 0;
    coreVerts[i++] = p.pos.x;
    coreVerts[i++] = p.pos.y;
    // Brighten core color for more intensity
    coreVerts[i++] = std::min(1.0f, p.color.r * 1.3f);
    coreVerts[i++] = std::min(1.0f, p.color.g * 1.3f);
    coreVerts[i++] = std::min(1.0f, p.color.b * 1.3f);
    coreVerts[i++] = 1.0f;  // Fully opaque core
    
    for (int s = 0; s <= SEGMENTS; ++s) {
        float a = (float)s / (float)SEGMENTS * 6.283185307f;
        coreVerts[i++] = p.pos.x + p.radius * std::cos(a);
        coreVerts[i++] = p.pos.y + p.radius * std::sin(a);
        coreVerts[i++] = std::min(1.0f, p.color.r * 1.3f);
        coreVerts[i++] = std::min(1.0f, p.color.g * 1.3f);
        coreVerts[i++] = std::min(1.0f, p.color.b * 1.3f);
        coreVerts[i++] = 0.8f;  // Slight fade at edge for smoothness
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * i), coreVerts, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Renderer::drawDragPreview(Vec2 from, Vec2 to) {
    float verts[2 * 6] = {
        from.x, from.y, 1.0f, 1.0f, 0.6f, 0.8f,
        to.x,   to.y,   1.0f, 1.0f, 0.6f, 0.8f
    };
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));

    glUseProgram(program_);
    float W = (float)width_;
    float H = (float)height_;
    float proj[16] = {
        2.0f/W, 0, 0, 0,
        0, -2.0f/H, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUniformMatrix4fv(glGetUniformLocation(program_, "uProj"), 1, GL_FALSE, proj);
    glDrawArrays(GL_LINES, 0, 2);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}
