# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)
project(image_transfer_regression LANGUAGES CXX)
enable_testing()
set(VULKAN_HEADERS_PATH "${CMAKE_CURRENT_LIST_DIR}/../../externals/vulkan-headers/include"
    CACHE PATH "Vulkan headers (no Vulkan driver required)")
add_executable(image_transfer_test test_image_transfer.cpp)
target_compile_features(image_transfer_test PRIVATE cxx_std_23)
target_include_directories(image_transfer_test PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../src" "${VULKAN_HEADERS_PATH}")
add_test(NAME image_transfer_depth_formats COMMAND image_transfer_test formats)
add_test(NAME image_transfer_depth_regions COMMAND image_transfer_test regions)
add_test(NAME image_transfer_transition_order COMMAND image_transfer_test transitions)
