# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.22)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(modgen VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_definitions( -DMODULE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/libraries/" )
add_definitions( -DLIBDOC_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../externals/ps4libdoc/system/common/lib/" )

add_executable(modgen
    main.cpp
    json.hpp
)

