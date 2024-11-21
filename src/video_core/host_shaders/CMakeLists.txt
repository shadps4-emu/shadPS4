# SPDX-FileCopyrightText: Copyright 2023 Citra Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(SHADER_FILES
    detile_m8x1.comp
    detile_m8x2.comp
    detile_m32x1.comp
    detile_m32x2.comp
    detile_m32x4.comp
    fs_tri.vert
    post_process.frag
)

set(SHADER_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)
set(SHADER_DIR ${SHADER_INCLUDE}/video_core/host_shaders)
set(HOST_SHADERS_INCLUDE ${SHADER_INCLUDE} PARENT_SCOPE)

set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/source_shader.h.in)
set(HEADER_GENERATOR ${CMAKE_CURRENT_SOURCE_DIR}/StringShaderHeader.cmake)

foreach(FILENAME IN ITEMS ${SHADER_FILES})
    string(REPLACE "." "_" SHADER_NAME ${FILENAME})
    set(SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME})
    set(SOURCE_HEADER_FILE ${SHADER_DIR}/${SHADER_NAME}.h)
    add_custom_command(
        OUTPUT
            ${SOURCE_HEADER_FILE}
        COMMAND
            ${CMAKE_COMMAND} -P ${HEADER_GENERATOR} ${SOURCE_FILE} ${SOURCE_HEADER_FILE} ${INPUT_FILE}
        MAIN_DEPENDENCY
            ${SOURCE_FILE}
        DEPENDS
            ${INPUT_FILE}
            # HEADER_GENERATOR should be included here but msbuild seems to assume it's always modified
    )
    set(SHADER_HEADERS ${SHADER_HEADERS} ${SOURCE_HEADER_FILE})
endforeach()

set(SHADER_SOURCES ${SHADER_FILES})
list(APPEND SHADER_SOURCES ${GLSL_INCLUDES})

add_custom_target(host_shaders
    DEPENDS
        ${SHADER_HEADERS}
    SOURCES
        ${SHADER_SOURCES}
)
