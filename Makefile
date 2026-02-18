# Particle Sandbox — build with make

CXX     := clang++
SRCDIR  := src
SOURCES := $(SRCDIR)/main.cpp $(SRCDIR)/App.cpp $(SRCDIR)/Simulation.cpp $(SRCDIR)/Renderer.cpp
TARGET  := particle_sandbox

# SDL2: use pkg-config if available, else Homebrew paths on Mac
SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS   := $(shell pkg-config --libs sdl2 2>/dev/null)
SDL2_TTF_CFLAGS := $(shell pkg-config --cflags SDL2_ttf 2>/dev/null)
SDL2_TTF_LIBS   := $(shell pkg-config --libs SDL2_ttf 2>/dev/null)
ifeq ($(SDL2_CFLAGS),)
  # Homebrew: headers are in include/SDL2/, so we need parent dir and #include <SDL2/SDL.h>
  SDL2_CFLAGS := -I/opt/homebrew/include -I/usr/local/include
  SDL2_LIBS   := -L/opt/homebrew/lib -L/usr/local/lib -lSDL2
  SDL2_TTF_CFLAGS := -I/opt/homebrew/include/SDL2 -I/usr/local/include/SDL2
  SDL2_TTF_LIBS   := -L/opt/homebrew/lib -L/usr/local/lib -lSDL2_ttf
endif

# OpenGL: framework on macOS
UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
  CXXFLAGS += -I/opt/homebrew/include -I/usr/local/include
  LDFLAGS  += $(SDL2_LIBS) $(SDL2_TTF_LIBS) -framework OpenGL -framework CoreFoundation
else
  LDFLAGS  += $(SDL2_LIBS) $(SDL2_TTF_LIBS) -lGL
endif

CXXFLAGS += -std=c++17 -Wall -I$(SRCDIR) $(SDL2_CFLAGS) $(SDL2_TTF_CFLAGS)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
