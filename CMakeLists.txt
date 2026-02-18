cmake_minimum_required(VERSION 3.14)
project(particle_sandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(particle_sandbox
    src/main.cpp
    src/App.cpp
    src/Simulation.cpp
    src/Renderer.cpp
)

target_include_directories(particle_sandbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(particle_sandbox
    SDL2::SDL2
    OpenGL::GL
)

if(APPLE)
    target_link_libraries(particle_sandbox
        "-framework OpenGL"
        "-framework CoreFoundation"
    )
endif()
