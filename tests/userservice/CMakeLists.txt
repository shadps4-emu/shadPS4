# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)
project(userservice_startup_regression LANGUAGES CXX)
enable_testing()
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(userservice_user_color_test
    "${REPO_ROOT}/src/core/libraries/system/userservice_user_color.cpp"
    test_user_color.cpp)
target_compile_features(userservice_user_color_test PRIVATE cxx_std_20)
target_include_directories(userservice_user_color_test PRIVATE
    "${REPO_ROOT}/src"
    "${REPO_ROOT}/externals/fmt/include"
    "${REPO_ROOT}/externals/json/include")
add_test(NAME userservice_user_color_test COMMAND userservice_user_color_test)
