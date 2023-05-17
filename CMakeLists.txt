cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(shadps4)

include_directories(third-party/)
include_directories(third-party/imgui/)
include_directories(third-party/imgui/backends)
include_directories(third-party/sdl/)
include_directories(third-party/fmt/include)
include_directories(third-party/magic_enum/include)
add_subdirectory("third-party")

#=================== EXAMPLE ===================

add_executable(shadps4
    src/main.cpp
    src/types.h
    src/Core/FsFile.cpp
    src/Core/FsFile.h
    src/Loader/Elf.cpp
    src/Loader/Elf.h
    src/GUI/ElfViewer.cpp
    src/GUI/ElfViewer.h "src/Util/Log.h" "src/Util/Log.cpp" "src/Core/Memory.cpp" "src/Core/Memory.h")

find_package(OpenGL REQUIRED)

target_link_libraries(shadps4 PUBLIC fmt spdlog IMGUI SDL3-shared ${OPENGL_LIBRARY})

add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     $<TARGET_FILE:SDL3-shared>
     $<TARGET_FILE_DIR:shadps4>)