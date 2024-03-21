# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

if (MSVC)
    # Silence "deprecation" warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

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

if(NOT ENABLE_QT_GUI)
# SDL3
add_subdirectory(sdl3 EXCLUDE_FROM_ALL)
endif()
