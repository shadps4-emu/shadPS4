# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)
project(userservice_startup_regression LANGUAGES CXX)
enable_testing()
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(USERSERVICE_SOURCE "${REPO_ROOT}/src/core/libraries/system/userservice.cpp"
    CACHE FILEPATH "Production UserService source for the regression test")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${USERSERVICE_SOURCE}")
file(READ "${USERSERVICE_SOURCE}" USERSERVICE_TEXT)
string(FIND "${USERSERVICE_TEXT}" "s32 PS4_SYSV_ABI sceUserServiceGetUserColor(" COLOR_BEGIN)
string(FIND "${USERSERVICE_TEXT}" "int PS4_SYSV_ABI sceUserServiceGetUserGroupName(" COLOR_END)
if(COLOR_BEGIN LESS 0 OR COLOR_END LESS COLOR_BEGIN)
    message(FATAL_ERROR "Cannot locate production sceUserServiceGetUserColor for the regression test")
endif()
math(EXPR COLOR_LENGTH "${COLOR_END} - ${COLOR_BEGIN}")
string(SUBSTRING "${USERSERVICE_TEXT}" ${COLOR_BEGIN} ${COLOR_LENGTH} COLOR_BODY)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/userservice_user_color.inc" "${COLOR_BODY}")
add_executable(userservice_user_color_test test_user_color.cpp)
target_compile_features(userservice_user_color_test PRIVATE cxx_std_20)
target_include_directories(userservice_user_color_test PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${REPO_ROOT}/src"
    "${REPO_ROOT}/externals/fmt/include"
    "${REPO_ROOT}/externals/json/include")
add_test(NAME userservice_user_color_test COMMAND userservice_user_color_test)
