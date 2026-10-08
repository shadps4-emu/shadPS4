# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(Threads REQUIRED)

add_executable(shadps4_startup_loading_test
    test_startup_loading.cpp
    ${CMAKE_SOURCE_DIR}/src/imgui/startup_loading.cpp
    ${CMAKE_SOURCE_DIR}/externals/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/externals/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/externals/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/externals/imgui/imgui_widgets.cpp
)

target_compile_features(shadps4_startup_loading_test PRIVATE cxx_std_23)
target_compile_definitions(shadps4_startup_loading_test
    PRIVATE IMGUI_USER_CONFIG="imgui/imgui_config.h"
)
target_include_directories(shadps4_startup_loading_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/externals/imgui
)
target_link_libraries(shadps4_startup_loading_test PRIVATE Threads::Threads)

add_test(NAME startup_loading_lifecycle COMMAND shadps4_startup_loading_test lifecycle)
add_test(NAME startup_loading_black_frames COMMAND shadps4_startup_loading_test black_frames)
add_test(NAME startup_loading_ui COMMAND shadps4_startup_loading_test ui)
