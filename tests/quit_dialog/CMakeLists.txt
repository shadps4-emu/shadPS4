# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)
project(quit_dialog_regression LANGUAGES CXX)
enable_testing()
set(SHADPS4_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../src")
set(SDL_HEADERS_PATH "${CMAKE_CURRENT_LIST_DIR}/../../externals/sdl3/include"
    CACHE PATH "Headers from the pinned SDL submodule; no display or SDL library required")
set(ROUTING_SOURCE_PATH "${SHADPS4_SOURCE}/sdl_window.cpp" CACHE FILEPATH "SDL event routing source")
function(extract name source first last suffix)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
    file(READ "${source}" body)
    string(FIND "${body}" "${first}" begin)
    string(FIND "${body}" "${last}" end)
    if(begin LESS 0 OR end LESS begin)
        message(FATAL_ERROR "Cannot extract production ${name}")
    endif()
    math(EXPR length "${end} - ${begin}")
    string(SUBSTRING "${body}" ${begin} ${length} body)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${name}.inc" "${body}${suffix}")
endfunction()
extract(event_dispatch "${ROUTING_SOURCE_PATH}" "void WindowSDL::WaitEvent() {"
    "    switch (event.type) {" "    ++guest_events;\n}\n")
extract(quit_actions "${SHADPS4_SOURCE}/core/devtools/layer.cpp"
    "static void ApplyQuitAction(" "void ShowVolume()" "")
extract(pad_capture "${SHADPS4_SOURCE}/imgui/renderer/imgui_core.cpp"
    "bool IsGamepadInputCaptured() {" "void Initialize(" "")
add_executable(quit_dialog_test test_quit_dialog.cpp)
target_compile_features(quit_dialog_test PRIVATE cxx_std_20)
target_include_directories(quit_dialog_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    "${SHADPS4_SOURCE}" "${SDL_HEADERS_PATH}")
foreach(case second_controller unfocused_confirm cancel_resume held_confirm keyboard hotplug)
    add_test(NAME quit_dialog_${case} COMMAND quit_dialog_test ${case})
endforeach()
