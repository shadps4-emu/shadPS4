# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

if (MSVC)
    # Silence "deprecation" warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

# Boost
set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/externals/boost" CACHE STRING "")
set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/externals/boost" CACHE STRING "")
set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "")
add_library(boost INTERFACE)
target_include_directories(boost SYSTEM INTERFACE ${Boost_INCLUDE_DIR})

# fmtlib
add_subdirectory(fmt EXCLUDE_FROM_ALL)

# Discord-RPC
set(BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
target_include_directories(discord-rpc INTERFACE ./discord-rpc/include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND MSVC)
    # If it is clang and MSVC we will add a static lib
    # CryptoPP
    add_subdirectory(cryptoppwin EXCLUDE_FROM_ALL)
    target_include_directories(cryptoppwin INTERFACE cryptoppwin/include)
else()
    # CryptoPP
    set(CRYPTOPP_BUILD_TESTING OFF)
    set(CRYPTOPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cryptopp/)
    add_subdirectory(cryptopp-cmake EXCLUDE_FROM_ALL)
endif()

# Zlib-Ng
set(ZLIB_ENABLE_TESTS OFF)
set(WITH_GTEST OFF)
set(WITH_NEW_STRATEGIES ON)
set(WITH_NATIVE_INSTRUCTIONS ON)
add_subdirectory(zlib-ng)

# SDL3
add_subdirectory(sdl3 EXCLUDE_FROM_ALL)

# vulkan-headers
add_library(vulkan-headers INTERFACE)
target_include_directories(vulkan-headers SYSTEM INTERFACE ./vulkan-headers/include)

# VMA
add_library(vma INTERFACE)
target_include_directories(vma SYSTEM INTERFACE ./vma/include)

# glslang
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "")
set(ENABLE_CTEST OFF CACHE BOOL "")
set(ENABLE_HLSL OFF CACHE BOOL "")
set(BUILD_EXTERNAL OFF CACHE BOOL "")
set(ENABLE_OPT OFF CACHE BOOL "")
add_subdirectory(glslang)

# Robin-map
add_subdirectory(robin-map EXCLUDE_FROM_ALL)

# Xbyak
add_subdirectory(xbyak EXCLUDE_FROM_ALL)

# MagicEnum
add_subdirectory(magic_enum EXCLUDE_FROM_ALL)

# Toml11
add_subdirectory(toml11 EXCLUDE_FROM_ALL)

# xxHash
add_library(xxhash xxhash/xxhash.h xxhash/xxhash.c)
target_include_directories(xxhash PUBLIC xxhash)

# Zydis
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory(zydis EXCLUDE_FROM_ALL)

# Winpthreads
if (WIN32)
    add_subdirectory(winpthreads EXCLUDE_FROM_ALL)
    target_include_directories(winpthreads INTERFACE winpthreads/include)
endif()

# sirit
add_subdirectory(sirit EXCLUDE_FROM_ALL)
if (WIN32)
    target_compile_options(sirit PUBLIC "-Wno-error=unused-command-line-argument")
endif()
