cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(shadps4)

# This function should be passed a list of all files in a target. It will automatically generate
# file groups following the directory hierarchy, so that the layout of the files in IDEs matches the
# one in the filesystem.
function(create_target_directory_groups target_name)
    # Place any files that aren't in the source list in a separate group so that they don't get in
    # the way.
    source_group("Other Files" REGULAR_EXPRESSION ".")

    get_target_property(target_sources "${target_name}" SOURCES)

    foreach(file_name IN LISTS target_sources)
        get_filename_component(dir_name "${file_name}" PATH)
        # Group names use '\' as a separator even though the entire rest of CMake uses '/'...
        string(REPLACE "/" "\\" group_name "${dir_name}")
        source_group("${group_name}" FILES "${file_name}")
    endforeach()
endfunction()

add_subdirectory(third-party)
include_directories(src)

set(LIBC_SOURCES src/core/hle/libraries/libc/libc.cpp
                 src/core/hle/libraries/libc/libc.h
                 src/core/hle/libraries/libc/printf.h
                 src/core/hle/libraries/libc/va_ctx.h
                 src/core/hle/libraries/libc/libc_cxa.cpp
                 src/core/hle/libraries/libc/libc_cxa.h
                 src/core/hle/libraries/libc/libc_stdio.cpp
                 src/core/hle/libraries/libc/libc_stdio.h
                 src/core/hle/libraries/libc/libc_math.cpp
                 src/core/hle/libraries/libc/libc_math.h
                 src/core/hle/libraries/libc/libc_string.cpp
                 src/core/hle/libraries/libc/libc_string.h
                 src/core/hle/libraries/libc/libc_stdlib.cpp
                 src/core/hle/libraries/libc/libc_stdlib.h
)
set(USERSERVICE_SOURCES src/core/hle/libraries/libuserservice/libuserservice.cpp
                        src/core/hle/libraries/libuserservice/libuserservice.h
)

set(PAD_SOURCES src/core/hle/libraries/libpad/pad.cpp
                src/core/hle/libraries/libpad/pad.h
)

set(SYSTEMSERVICE_SOURCES src/core/hle/libraries/libsystemservice/system_service.cpp
                          src/core/hle/libraries/libsystemservice/system_service.h
)

set(FILESYSTEM_SOURCES src/core/hle/libraries/libkernel/file_system.cpp
                       src/core/hle/libraries/libkernel/file_system.h
                       src/core/file_sys/fs.cpp
                       src/core/file_sys/fs.h
)

set(HOST_SOURCES src/Emulator/Host/controller.cpp
                 src/Emulator/Host/controller.h
)

add_executable(shadps4
    src/common/debug.h
    src/common/disassembler.cpp
    src/common/disassembler.h
    src/common/discord.cpp
    src/common/discord.h
    src/common/fs_file.cpp
    src/common/fs_file.h
    src/common/log.cpp
    src/common/log.h
    src/common/native_clock.cpp
    src/common/native_clock.h
    src/common/rdtsc.cpp
    src/common/rdtsc.h
    src/common/singleton.h
    src/common/string_util.cpp
    src/common/string_util.h
    src/common/types.h
    src/common/uint128.h
    src/common/version.h
    ${LIBC_SOURCES}
    ${USERSERVICE_SOURCES}
    ${PAD_SOURCES}
    ${SYSTEMSERVICE_SOURCES}
    ${FILESYSTEM_SOURCES}
    ${HOST_SOURCES}
    src/main.cpp
    src/core/loader/elf.cpp
    src/core/loader/elf.h
    src/GUI/ElfViewer.cpp
    src/GUI/ElfViewer.h 
    src/Util/config.cpp
    src/Util/config.h
    src/core/virtual_memory.cpp
    src/core/virtual_memory.h
    src/core/linker.cpp
    src/core/linker.h
    src/core/aerolib/stubs.cpp
    src/core/aerolib/stubs.h
    src/core/aerolib/aerolib.cpp
    src/core/aerolib/aerolib.h
    src/core/hle/kernel/Objects/physical_memory.h
    src/core/hle/kernel/Objects/physical_memory.cpp
    src/core/PS4/HLE/Graphics/video_out.cpp
    src/core/PS4/HLE/Graphics/video_out.h
    src/core/hle/kernel/event_queues.cpp
    src/core/hle/kernel/event_queues.h
    src/core/hle/kernel/cpu_management.cpp
    src/core/hle/kernel/cpu_management.h
    src/core/loader/symbols_resolver.h
    src/core/loader/symbols_resolver.cpp
    src/core/hle/libraries/libs.cpp
    src/core/hle/libraries/libs.h
    src/core/hle/libraries/libkernel/libkernel.cpp
    src/core/hle/libraries/libkernel/libkernel.h
    src/core/hle/libraries/libscegnmdriver/libscegnmdriver.cpp
    src/core/hle/libraries/libscegnmdriver/libscegnmdriver.h
    src/core/hle/libraries/libkernel/thread_management.cpp
    src/core/hle/libraries/libkernel/thread_management.h
    src/core/hle/kernel/memory_management.cpp
    src/core/hle/kernel/memory_management.h
    src/core/hle/error_codes.h
    src/core/PS4/GPU/gpu_memory.cpp
    src/core/PS4/GPU/gpu_memory.h
    src/emulator.cpp
    src/emulator.h
    src/core/hle/kernel/Objects/event_queue.h
    src/core/hle/kernel/Objects/event_queue.cpp
    src/core/PS4/HLE/Graphics/Objects/video_out_ctx.cpp
    src/core/PS4/HLE/Graphics/Objects/video_out_ctx.h
    src/core/PS4/HLE/Graphics/graphics_ctx.h
    src/vulkan_util.cpp
    src/vulkan_util.h
    src/core/PS4/GPU/video_out_buffer.cpp
    src/core/PS4/GPU/video_out_buffer.h
    src/core/PS4/HLE/Graphics/graphics_render.cpp
    src/core/PS4/HLE/Graphics/graphics_render.h
    src/core/PS4/GPU/tile_manager.cpp
    src/core/PS4/GPU/tile_manager.h
    src/core/hle/libraries/libkernel/time_management.cpp
    src/core/hle/libraries/libkernel/time_management.h
 "src/common/io_file.cpp" "src/common/io_file.h")

create_target_directory_groups(shadps4)

target_link_libraries(shadps4 PRIVATE magic_enum::magic_enum fmt::fmt spdlog::spdlog toml11::toml11)
target_link_libraries(shadps4 PRIVATE discord-rpc imgui SDL3-shared vulkan-1 xxhash Zydis)
if (WIN32)
    target_link_libraries(shadps4 PRIVATE mincore winpthread)
endif()

add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     $<TARGET_FILE:SDL3-shared>
     $<TARGET_FILE_DIR:shadps4>)
if (WIN32)
 add_custom_command(TARGET shadps4 POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_if_different
     "${PROJECT_SOURCE_DIR}/third-party/winpthread/bin/libwinpthread-1.dll" $<TARGET_FILE_DIR:shadps4>)
endif()
