cmake_minimum_required(VERSION 3.15)
project(raycaster)

set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(include)

# Recursively find .cpp files
file(GLOB SRC_FILES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/*/*.cpp"
	"${PROJECT_SOURCE_DIR}/src/*/*/*.cpp"
)

add_executable(raycaster ${SRC_FILES})

# macOS OpenGL + GLUT
target_link_libraries(raycaster
    "-framework OpenGL"
    "-framework GLUT"
)
