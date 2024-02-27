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


