cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(shadps4)

include_directories(third-party/)
include_directories(third-party/discord-rpc/include)
include_directories(third-party/imgui/)
include_directories(third-party/imgui/backends)
include_directories(third-party/sdl/)
include_directories(third-party/fmt/include)
include_directories(third-party/magic_enum/include)
include_directories(third-party/zydis/include/Zydis)
include_directories(third-party/winpthread/include)
add_subdirectory("third-party")
#=================== EXAMPLE ===================
include_directories(src)

add_executable(shadps4
    src/main.cpp
    src/types.h
    src/Core/FsFile.cpp
    src/Core/FsFile.h
    src/Core/PS4/Loader/Elf.cpp
    src/Core/PS4/Loader/Elf.h
    src/GUI/ElfViewer.cpp
    src/GUI/ElfViewer.h 
    src/Util/log.h
    src/Util/log.cpp
    src/Util/config.cpp
    src/Util/config.h
    src/Core/virtual_memory.cpp
    src/Core/virtual_memory.h
    src/Core/PS4/Linker.cpp
    src/Core/PS4/Linker.h
    src/Lib/Threads.cpp
    src/Lib/Threads.h
    src/Core/PS4/HLE/Kernel/Objects/physical_memory.h
    src/Core/PS4/HLE/Kernel/Objects/physical_memory.cpp
    src/Util/string_util.cpp
    src/Util/string_util.cpp
    src/Core/PS4/HLE/Graphics/video_out.cpp
    src/Core/PS4/HLE/Graphics/video_out.h
    src/discord.h
    src/discord.cpp
    src/Core/PS4/HLE/Kernel/event_queues.cpp
    src/Core/PS4/HLE/Kernel/event_queues.h
    src/Core/PS4/HLE/Kernel/cpu_management.cpp
    src/Core/PS4/HLE/Kernel/cpu_management.h
     "src/Util/Singleton.h" "src/Util/Disassembler.cpp" "src/Util/Disassembler.h" "src/Core/PS4/Util/aerolib.h" "src/Core/PS4/Loader/SymbolsResolver.h" "src/Core/PS4/Loader/SymbolsResolver.cpp" "src/Core/PS4/HLE/Libs.cpp" "src/Core/PS4/HLE/Libs.h" "src/Core/PS4/HLE/LibC.cpp" "src/Core/PS4/HLE/LibC.h" "src/Lib/Timer.cpp" "src/Lib/Timer.h" "src/Core/PS4/HLE/LibKernel.cpp" "src/Core/PS4/HLE/LibKernel.h"   "src/Core/PS4/HLE/LibSceGnmDriver.cpp" "src/Core/PS4/HLE/LibSceGnmDriver.h" "src/Core/PS4/HLE/Kernel/ThreadManagement.cpp" "src/Core/PS4/HLE/Kernel/ThreadManagement.h" "src/Core/PS4/HLE/ErrorCodes.h" "src/debug.h" "src/Core/PS4/HLE/Kernel/memory_management.cpp" "src/Core/PS4/HLE/Kernel/memory_management.h" "src/Core/PS4/GPU/gpu_memory.cpp" "src/Core/PS4/GPU/gpu_memory.h" "src/emulator.cpp" "src/emulator.h"  "src/Core/PS4/HLE/Kernel/Objects/event_queue.h" "src/Core/PS4/HLE/Kernel/Objects/event_queue.cpp" "src/Core/PS4/HLE/Graphics/Objects/video_out_ctx.cpp" "src/Core/PS4/HLE/Graphics/Objects/video_out_ctx.h" "src/Core/PS4/HLE/Graphics/graphics_ctx.h")

find_package(OpenGL REQUIRED)
target_link_libraries(shadps4 PUBLIC fmt mincore spdlog IMGUI SDL3-shared ${OPENGL_LIBRARY})

add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     $<TARGET_FILE:SDL3-shared>
     $<TARGET_FILE_DIR:shadps4>)
add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     "${PROJECT_SOURCE_DIR}/third-party/winpthread/bin/libwinpthread-1.dll" $<TARGET_FILE_DIR:shadps4>)
