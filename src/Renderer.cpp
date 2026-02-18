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
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);  // Deep space black with slight blue tint
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawParticles(const std::vector<Particle>& particles) {
    glUseProgram(program_);
    for (const Particle& p : particles) {
        drawCircle(p);
    }
}

void Renderer::drawParticleTrails(const std::vector<Particle>& particles) {
    float proj[16] = {
        2.0f / (float)width_, 0, 0, 0,
        0, -2.0f / (float)height_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUseProgram(program_);
    glUniformMatrix4fv(glGetUniformLocation(program_, "uProj"), 1, GL_FALSE, proj);
    
    for (const Particle& p : particles) {
        if (p.trailLength < 2) continue;
        
        // Draw trail as connected segments with fading alpha
        for (int i = 0; i < p.trailLength - 1; ++i) {
            int idx1 = (p.trailIndex - p.trailLength + i + Particle::MAX_TRAIL_LENGTH) % Particle::MAX_TRAIL_LENGTH;
            int idx2 = (p.trailIndex - p.trailLength + i + 1 + Particle::MAX_TRAIL_LENGTH) % Particle::MAX_TRAIL_LENGTH;
            Vec2 a = p.trail[idx1];
            Vec2 b = p.trail[idx2];
            
            float alpha = (float)i / (float)p.trailLength;
            float trailWidth = 3.0f + alpha * 5.0f;  // Thicker trails
            float r = p.color.r * (0.5f + 0.5f * alpha);
            float g = p.color.g * (0.5f + 0.5f * alpha);
            float bl = p.color.b * (0.5f + 0.5f * alpha);
            
            // Draw segment as a quad (two triangles)
            Vec2 dir = b - a;
            float len = dir.length();
            if (len < 0.1f) continue;
            Vec2 perp = Vec2(-dir.y, dir.x) * (trailWidth / len);
            
            float fadeAlpha = alpha * 0.7f;  // More visible trails
            float quadVerts[6 * 6] = {
                a.x + perp.x, a.y + perp.y, r, g, bl, fadeAlpha,
                a.x - perp.x, a.y - perp.y, r, g, bl, fadeAlpha,
                b.x + perp.x, b.y + perp.y, r, g, bl, fadeAlpha * 0.5f,
                b.x + perp.x, b.y + perp.y, r, g, bl, fadeAlpha * 0.5f,
                a.x - perp.x, a.y - perp.y, r, g, bl, fadeAlpha,
                b.x - perp.x, b.y - perp.y, r, g, bl, fadeAlpha * 0.5f
            };
            
            unsigned int vao, vbo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }
    }
}

void Renderer::drawGravityWells(const std::vector<GravityWell>& wells) {
    const int SEGMENTS = 32;
    float proj[16] = {
        2.0f / (float)width_, 0, 0, 0,
        0, -2.0f / (float)height_, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    glUseProgram(program_);
    glUniformMatrix4fv(glGetUniformLocation(program_, "uProj"), 1, GL_FALSE, proj);

    for (const GravityWell& well : wells) {
        // Outer glow (large, very transparent)
        float outerRadius = 60.0f;
        float outerVerts[(SEGMENTS + 2) * 6];
        int i = 0;
        outerVerts[i++] = well.pos.x;
        outerVerts[i++] = well.pos.y;
        outerVerts[i++] = 0.2f;
        outerVerts[i++] = 0.4f;
        outerVerts[i++] = 1.0f;
        outerVerts[i++] = 0.15f;
        for (int s = 0; s <= SEGMENTS; ++s) {
            float a = (float)s / (float)SEGMENTS * 6.283185307f;
            outerVerts[i++] = well.pos.x + outerRadius * std::cos(a);
            outerVerts[i++] = well.pos.y + outerRadius * std::sin(a);
            outerVerts[i++] = 0.2f;
            outerVerts[i++] = 0.4f;
            outerVerts[i++] = 1.0f;
            outerVerts[i++] = 0.0f;
        }
        unsigned int vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * i), outerVerts, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        
        // Middle glow
        float midRadius = 35.0f;
        i = 0;
        float midVerts[(SEGMENTS + 2) * 6];
        midVerts[i++] = well.pos.x;
        midVerts[i++] = well.pos.y;
        midVerts[i++] = 0.3f;
        midVerts[i++] = 0.5f;
        midVerts[i++] = 1.0f;
        midVerts[i++] = 0.4f;
        for (int s = 0; s <= SEGMENTS; ++s) {
            float a = (float)s / (float)SEGMENTS * 6.283185307f;
            midVerts[i++] = well.pos.x + midRadius * std::cos(a);
            midVerts[i++] = well.pos.y + midRadius * std::sin(a);
            midVerts[i++] = 0.3f;
            midVerts[i++] = 0.5f;
            midVerts[i++] = 1.0f;
            midVerts[i++] = 0.0f;
        }
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * i), midVerts, GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
        glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        
        // Bright core
        float coreRadius = 18.0f;
        i = 0;
        float coreVerts[(SEGMENTS + 2) * 6];
        coreVerts[i++] = well.pos.x;
        coreVerts[i++] = well.pos.y;
        coreVerts[i++] = 0.4f;
        coreVerts[i++] = 0.7f;
        coreVerts[i++] = 1.0f;
        coreVerts[i++] = 1.0f;
        for (int s = 0; s <= SEGMENTS; ++s) {
            float a = (float)s / (float)SEGMENTS * 6.283185307f;
            coreVerts[i++] = well.pos.x + coreRadius * std::cos(a);
            coreVerts[i++] = well.pos.y + coreRadius * std::sin(a);
            coreVerts[i++] = 0.4f;
            coreVerts[i++] = 0.7f;
            coreVerts[i++] = 1.0f;
            coreVerts[i++] = 0.8f;
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
    float glowRadius = p.radius * 3.5f;  // Larger glow for more intensity
    float glowAlpha = 0.4f;  // More visible glow
    
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
    coreVerts[i++] = std::min(1.0f, p.color.r * 1.5f);
    coreVerts[i++] = std::min(1.0f, p.color.g * 1.5f);
    coreVerts[i++] = std::min(1.0f, p.color.b * 1.5f);
    coreVerts[i++] = 1.0f;  // Fully opaque core
    
    for (int s = 0; s <= SEGMENTS; ++s) {
        float a = (float)s / (float)SEGMENTS * 6.283185307f;
        coreVerts[i++] = p.pos.x + p.radius * std::cos(a);
        coreVerts[i++] = p.pos.y + p.radius * std::sin(a);
        coreVerts[i++] = std::min(1.0f, p.color.r * 1.5f);
        coreVerts[i++] = std::min(1.0f, p.color.g * 1.5f);
        coreVerts[i++] = std::min(1.0f, p.color.b * 1.5f);
        coreVerts[i++] = 0.9f;  // Less fade for brighter edge
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
