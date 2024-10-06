# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

project(ImGui_Resources)

add_executable(Dear_ImGui_FontEmbed ${CMAKE_SOURCE_DIR}/externals/dear_imgui/misc/fonts/binary_to_compressed_c.cpp)

set(FONT_LIST
    NotoSansJP-Regular.ttf
    ProggyVector-Regular.ttf
)

set(OutputList "")
FOREACH (FONT_FILE ${FONT_LIST})
    string(REGEX REPLACE "-" "_" fontname ${FONT_FILE})
    string(TOLOWER ${fontname} fontname)
    string(REGEX REPLACE ".ttf" "" fontname_cpp ${fontname})
    set(fontname_cpp "imgui_font_${fontname_cpp}")

    MESSAGE(STATUS "Embedding font ${FONT_FILE}")
    set(OUTPUT "generated_fonts/imgui_fonts/${fontname}")
    add_custom_command(
        OUTPUT "${OUTPUT}.g.cpp"
        COMMAND ${CMAKE_COMMAND} -E make_directory "generated_fonts/imgui_fonts"
        COMMAND $<TARGET_FILE:Dear_ImGui_FontEmbed> -nostatic "${CMAKE_CURRENT_SOURCE_DIR}/fonts/${FONT_FILE}" ${fontname_cpp} > "${OUTPUT}.g.cpp"
        DEPENDS Dear_ImGui_FontEmbed "fonts/${FONT_FILE}"
        USES_TERMINAL
    )
    list(APPEND OutputList "${OUTPUT}.g.cpp")
ENDFOREACH ()

add_library(ImGui_Resources STATIC ${OutputList})
set(IMGUI_RESOURCES_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/generated_fonts PARENT_SCOPE)
