cmake_minimum_required(VERSION 3.16.3)

project(shadps4)

include_directories(third-party/)
include_directories(third-party/imgui/)
include_directories(third-party/imgui/backends)
include_directories(third-party/sdl/)
include_directories(third-party/fmt/include)

add_subdirectory("third-party")

#=================== EXAMPLE ===================

add_executable(shadps4
    src/main.cpp
)

find_package(OpenGL REQUIRED)

target_link_libraries(shadps4 PUBLIC fmt spdlog IMGUI SDL3-shared ${OPENGL_LIBRARY})