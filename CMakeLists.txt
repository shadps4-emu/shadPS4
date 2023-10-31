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
include_directories(third-party/vulkan/include)
include_directories(third-party/xxhash/include)
include_directories(third-party/result/include)
add_subdirectory("third-party")
#=================== EXAMPLE ===================
include_directories(src)

set(LIBC_SOURCES src/core/hle/libraries/Libc/Libc.cpp 
                 src/core/hle/libraries/Libc/Libc.h
                 src/core/hle/libraries/Libc/printf.h
                 src/core/hle/libraries/Libc/va_ctx.h
                 src/core/hle/libraries/Libc/libc_cxa.cpp
                 src/core/hle/libraries/Libc/libc_cxa.h
)
set(USERSERVICE_SOURCES src/core/hle/libraries/libuserservice/user_service.cpp
                        src/core/hle/libraries/libuserservice/user_service.h
)

set(PAD_SOURCES src/core/hle/libraries/libpad/pad.cpp
                src/core/hle/libraries/libpad/pad.h
)

set(SYSTEMSERVICE_SOURCES src/core/hle/libraries/libsystemservice/system_service.cpp
                          src/core/hle/libraries/libsystemservice/system_service.h
)

set(FILESYSTEM_SOURCES src/core/hle/libraries/libkernel/file_system.cpp
                       src/core/hle/libraries/libkernel/file_system.h
                       
                       
)

set(HOST_SOURCES src/Emulator/Host/controller.cpp
                 src/Emulator/Host/controller.h
)

set(UTIL_SOURCES src/Emulator/Util/singleton.h
)

add_executable(shadps4
    ${LIBC_SOURCES}
    ${USERSERVICE_SOURCES}
    ${PAD_SOURCES}
    ${SYSTEMSERVICE_SOURCES}
    ${FILESYSTEM_SOURCES}
    ${HOST_SOURCES}
    ${UTIL_SOURCES}
    src/Lib/Timer.cpp
    src/Lib/Timer.h
    src/main.cpp
    src/types.h
    src/core/FsFile.cpp
    src/core/FsFile.h
    src/core/PS4/Loader/Elf.cpp
    src/core/PS4/Loader/Elf.h
    src/GUI/ElfViewer.cpp
    src/GUI/ElfViewer.h 
    src/Util/log.h
    src/Util/log.cpp
    src/Util/config.cpp
    src/Util/config.h
    src/core/virtual_memory.cpp
    src/core/virtual_memory.h
    src/core/PS4/Linker.cpp
    src/core/PS4/Linker.h
    src/core/PS4/Stubs.cpp
    src/core/PS4/Stubs.h
    src/core/PS4/Util/aerolib.cpp
    src/core/PS4/HLE/Kernel/Objects/physical_memory.h
    src/core/PS4/HLE/Kernel/Objects/physical_memory.cpp
    src/Util/string_util.cpp
    src/Util/string_util.cpp
    src/core/PS4/HLE/Graphics/video_out.cpp
    src/core/PS4/HLE/Graphics/video_out.h
    src/discord.h
    src/discord.cpp
    src/core/PS4/HLE/Kernel/event_queues.cpp
    src/core/PS4/HLE/Kernel/event_queues.h
    src/core/PS4/HLE/Kernel/cpu_management.cpp
    src/core/PS4/HLE/Kernel/cpu_management.h
    src/core/PS4/HLE/LibC.cpp
    src/core/PS4/HLE/LibC.h
    
    "src/Util/Disassembler.cpp" "src/Util/Disassembler.h" "src/core/PS4/Util/aerolib.h" "src/core/PS4/Loader/SymbolsResolver.h" "src/core/PS4/Loader/SymbolsResolver.cpp" "src/core/PS4/HLE/Libs.cpp" "src/core/PS4/HLE/Libs.h" "src/core/PS4/HLE/LibKernel.cpp" "src/core/PS4/HLE/LibKernel.h"   "src/core/PS4/HLE/LibSceGnmDriver.cpp" "src/core/PS4/HLE/LibSceGnmDriver.h" "src/core/PS4/HLE/Kernel/ThreadManagement.cpp" "src/core/PS4/HLE/Kernel/ThreadManagement.h" "src/core/PS4/HLE/ErrorCodes.h" "src/debug.h" "src/core/PS4/HLE/Kernel/memory_management.cpp" "src/core/PS4/HLE/Kernel/memory_management.h" "src/core/PS4/GPU/gpu_memory.cpp" "src/core/PS4/GPU/gpu_memory.h" "src/emulator.cpp" "src/emulator.h"  "src/core/PS4/HLE/Kernel/Objects/event_queue.h" "src/core/PS4/HLE/Kernel/Objects/event_queue.cpp" "src/core/PS4/HLE/Graphics/Objects/video_out_ctx.cpp" "src/core/PS4/HLE/Graphics/Objects/video_out_ctx.h" "src/core/PS4/HLE/Graphics/graphics_ctx.h" "src/vulkan_util.cpp" "src/vulkan_util.h" "src/core/PS4/GPU/video_out_buffer.cpp" "src/core/PS4/GPU/video_out_buffer.h" "src/core/PS4/HLE/Graphics/graphics_render.cpp" "src/core/PS4/HLE/Graphics/graphics_render.h" "src/core/PS4/GPU/tile_manager.cpp" "src/core/PS4/GPU/tile_manager.h" "src/version.h"  "src/emuTimer.cpp" "src/emuTimer.h" "src/core/hle/libraries/libkernel/time_management.cpp" "src/core/hle/libraries/libkernel/time_management.h")

find_package(OpenGL REQUIRED)
target_link_libraries(shadps4 PUBLIC fmt mincore spdlog IMGUI SDL3-shared ${OPENGL_LIBRARY} vulkan-1 spirv-tools-opt spirv-tools)

add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     $<TARGET_FILE:SDL3-shared>
     $<TARGET_FILE_DIR:shadps4>)
add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     "${PROJECT_SOURCE_DIR}/third-party/winpthread/bin/libwinpthread-1.dll" $<TARGET_FILE_DIR:shadps4>)
