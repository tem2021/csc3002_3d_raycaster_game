cmake_minimum_required(VERSION 3.15)
project(raycaster)

set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(include)

# Explicitly list all source files (recommended for better IDE support and build reliability)
set(SRC_FILES
    src/main.cpp
    src/core/Game.cpp
    src/entities/Enemy.cpp
    src/entities/Player.cpp
    src/entities/Weapon.cpp
    src/input/InputManager.cpp
    src/rendering/Renderer.cpp
    src/rendering/TextureManager.cpp
    src/world/Map.cpp
    src/world/Raycaster.cpp
)

add_executable(raycaster ${SRC_FILES})

if (APPLE)
    # macOS OpenGL + GLUT
    target_link_libraries(raycaster
        "-framework OpenGL"
        "-framework GLUT"
    )
elseif (WIN32)
    # Windows MinGW + freeglut
    target_link_libraries(raycaster PRIVATE
        freeglut
        glu32
        opengl32
    )
elseif(UNIX)
    target_link_libraries(raycaster GL GLU glut)
endif()
