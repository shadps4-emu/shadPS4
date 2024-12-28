# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set_directory_properties(PROPERTIES
    EXCLUDE_FROM_ALL ON
    SYSTEM ON
)

# Set CMP0069 policy to "NEW" in order to ensure consistent behavior when building external targets with LTO enabled
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

if (MSVC)
    # Silence "deprecation" warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

# Boost
if (NOT TARGET Boost::headers)
    set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/externals/ext-boost" CACHE STRING "")
    set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "")
    add_subdirectory(ext-boost)
endif()

# fmtlib
if (NOT TARGET fmt::fmt)
    add_subdirectory(fmt)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND MSVC)
    # If it is clang and MSVC we will add a static lib
    # CryptoPP
    add_subdirectory(cryptoppwin)
    target_include_directories(cryptoppwin INTERFACE cryptoppwin/include)
else()
    # CryptoPP
    if (NOT TARGET cryptopp::cryptopp)
        set(CRYPTOPP_INSTALL OFF)
        set(CRYPTOPP_BUILD_TESTING OFF)
        set(CRYPTOPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cryptopp)
        add_subdirectory(cryptopp-cmake)
        file(COPY cryptopp DESTINATION cryptopp FILES_MATCHING PATTERN "*.h")
        # remove externals/cryptopp from include directories because it contains a conflicting zlib.h file
        set_target_properties(cryptopp PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/cryptopp")
    endif()
endif()

if (NOT TARGET FFmpeg::ffmpeg)
    add_subdirectory(ffmpeg-core)
    add_library(FFmpeg::ffmpeg ALIAS ffmpeg)
endif()

# LibAtrac9
file(GLOB LIBATRAC9_SOURCES LibAtrac9/C/src/*.c)
add_library(LibAtrac9 STATIC ${LIBATRAC9_SOURCES})
target_include_directories(LibAtrac9 INTERFACE LibAtrac9/C/src)

# zlib
if (NOT TARGET ZLIB::ZLIB)
    set(ZLIB_ENABLE_TESTS OFF)
    set(WITH_GTEST OFF)
    set(WITH_NEW_STRATEGIES ON)
    set(WITH_NATIVE_INSTRUCTIONS ON)
    set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        ZLIB
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng"
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlib)
    # libpng expects this variable to exist after its find_package(ZLIB)
    set(ZLIB_INCLUDE_DIRS "${FETCHCONTENT_BASE_DIR}/zlib-build")
endif()

# SDL3
if (NOT TARGET SDL3::SDL3)
    set(SDL_PIPEWIRE OFF)
    add_subdirectory(sdl3)
endif()

# vulkan-headers
if (NOT TARGET Vulkan::Headers)
    set(VULKAN_HEADERS_ENABLE_MODULE OFF)
    add_subdirectory(vulkan-headers)
endif()

# VMA
if (NOT TARGET GPUOpen::VulkanMemoryAllocator)
    add_subdirectory(vma)
endif()

# RenderDoc
if (NOT TARGET RenderDoc::API)
    add_library(renderdoc INTERFACE)
    target_include_directories(renderdoc INTERFACE ./renderdoc)
    add_library(RenderDoc::API ALIAS renderdoc)
endif()

# glslang
if (NOT TARGET glslang::glslang)
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "")
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
    set(ENABLE_SPVREMAPPER OFF CACHE BOOL "")
    set(ENABLE_CTEST OFF CACHE BOOL "")
    set(ENABLE_HLSL OFF CACHE BOOL "")
    set(BUILD_EXTERNAL OFF CACHE BOOL "")
    set(ENABLE_OPT OFF CACHE BOOL "")
    add_subdirectory(glslang)
    file(COPY glslang/SPIRV DESTINATION glslang/glslang FILES_MATCHING PATTERN "*.h")
    target_include_directories(glslang INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/glslang")
endif()

# Robin-map
if (NOT TARGET tsl::robin_map)
    add_subdirectory(robin-map)
endif()

# Xbyak
if (NOT TARGET xbyak::xbyak)
    add_subdirectory(xbyak)
endif()

# MagicEnum
if (NOT TARGET magic_enum::magic_enum)
    add_subdirectory(magic_enum)
endif()

# Toml11
if (NOT TARGET toml11::toml11)
    add_subdirectory(toml11)
endif()

# xxHash
if (NOT TARGET xxHash::xxhash)
    add_library(xxhash xxhash/xxhash.h xxhash/xxhash.c)
    target_include_directories(xxhash PUBLIC xxhash)
    add_library(xxHash::xxhash ALIAS xxhash)
endif()

# Zydis
if (NOT TARGET Zydis::Zydis)
    option(ZYDIS_BUILD_TOOLS "" OFF)
    option(ZYDIS_BUILD_EXAMPLES "" OFF)
    add_subdirectory(zydis)
endif()

# Winpthreads
if (WIN32)
    add_subdirectory(winpthreads)
    target_include_directories(winpthreads INTERFACE winpthreads/include)
endif()

# sirit
add_subdirectory(sirit)
if (WIN32)
    target_compile_options(sirit PUBLIC "-Wno-error=unused-command-line-argument")
endif()

# half
if (NOT TARGET half::half)
    add_library(half INTERFACE)
    target_include_directories(half INTERFACE half/include)
    add_library(half::half ALIAS half)
endif()

# libpng
if (NOT TARGET PNG::PNG)
    set(PNG_SHARED OFF CACHE BOOL "" FORCE)
    set(PNG_STATIC ON CACHE BOOL "" FORCE)
    set(PNG_TESTS OFF CACHE BOOL "" FORCE)
    set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(libpng)
    add_library(PNG::PNG ALIAS png_static)
endif()

# Dear ImGui
add_library(Dear_ImGui
        dear_imgui/imgui.cpp
        dear_imgui/imgui_demo.cpp
        dear_imgui/imgui_draw.cpp
        dear_imgui/imgui_internal.h
        dear_imgui/imgui_tables.cpp
        dear_imgui/imgui_widgets.cpp
)
target_include_directories(Dear_ImGui INTERFACE dear_imgui/)

# Tracy
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    option(TRACY_ENABLE "" OFF)
else()
    option(TRACY_ENABLE "" ON)
endif()
option(TRACY_NO_CRASH_HANDLER "" ON) # Otherwise texture cache exceptions will be treaten as a crash
option(TRACY_ON_DEMAND "" ON)
option(TRACY_NO_FRAME_IMAGE "" ON)
option(TRACY_FIBERS "" OFF) # For AmdGpu frontend profiling, disabled due to instability
option(TRACY_NO_SYSTEM_TRACING "" ON)
option(TRACY_NO_CALLSTACK "" ON)
option(TRACY_NO_CODE_TRANSFER "" ON)
option(TRACY_NO_SAMPLING "" ON)
option(TRACY_ONLY_LOCALHOST "" ON)
option(TRACY_NO_CONTEXT_SWITCH "" ON)
add_subdirectory(tracy)

# pugixml
if (NOT TARGET pugixml::pugixml)
    add_subdirectory(pugixml)
endif()

# Discord RPC
if (ENABLE_DISCORD_RPC)
    set(BUILD_EXAMPLES OFF)
    add_subdirectory(discord-rpc)
    target_include_directories(discord-rpc INTERFACE discord-rpc/include)
endif()

# GCN Headers
add_subdirectory(gcn)

# stb
if (NOT TARGET stb::headers)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE stb)
    add_library(stb::headers ALIAS stb)
endif()

# cubeb
if (NOT TARGET cubeb::cubeb)
    option(BUILD_TESTS "" OFF)
    option(BUILD_TOOLS "" OFF)
    option(BUNDLE_SPEEX "" ON)
    option(USE_SANITIZERS "" OFF)
    add_subdirectory(cubeb)
    add_library(cubeb::cubeb ALIAS cubeb)
endif()

# Apple-only dependencies
if (APPLE)
    # date
    if (NOT TARGET date::date-tz)
        option(BUILD_TZ_LIB "" ON)
        option(USE_SYSTEM_TZ_DB "" ON)
        add_subdirectory(date)
    endif()

    # MoltenVK
    if (NOT TARGET MoltenVK)
        add_subdirectory(MoltenVK)
    endif()
endif()
