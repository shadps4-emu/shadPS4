# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(cpu_id_source "${PROJECT_SOURCE_DIR}/src/core/cpu_id_translation")
set(cpu_id_bundle "${PROJECT_BINARY_DIR}/cpu-id-runtime")
if (DynamoRIO_DIR)
    find_package(DynamoRIO CONFIG REQUIRED)
    add_subdirectory(src/core/cpu_id_translation)
    get_filename_component(cpu_id_runtime "${DynamoRIO_DIR}/.." ABSOLUTE)
    if (NOT DynamoRIO_SOURCE_DIR)
        set(DynamoRIO_SOURCE_DIR "${cpu_id_runtime}")
    endif()
    add_custom_target(shadps4_cpu_id_bundle
        COMMAND "${CMAKE_COMMAND}" "-DRUNTIME=${cpu_id_runtime}"
            "-DSOURCE=${DynamoRIO_SOURCE_DIR}" "-DCLIENT=$<TARGET_FILE:shadps4_cpu_id>"
            "-DOUTPUT=${cpu_id_bundle}" -P "${cpu_id_source}/package.cmake"
        DEPENDS shadps4_cpu_id VERBATIM)
else()
    include(ExternalProject)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_program(cpu_id_ninja NAMES ninja ninja-build REQUIRED)
    set(cpu_id_runtime "${PROJECT_BINARY_DIR}/cpu-id-runtime-build")
    set(cpu_id_runtime_source "${PROJECT_BINARY_DIR}/cpu-id-runtime-source")
    ExternalProject_Add(shadps4_cpu_id_runtime
        SOURCE_DIR "${cpu_id_runtime_source}"
        DOWNLOAD_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND "${Python3_EXECUTABLE}" "${cpu_id_source}/build.py" --runtime-only
            --runtime-source "${cpu_id_runtime_source}" --runtime-build "${cpu_id_runtime}"
            --cmake "${CMAKE_COMMAND}" --ninja "${cpu_id_ninja}" --jobs 2
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND "")
    ExternalProject_Add(shadps4_cpu_id_bundle
        SOURCE_DIR "${cpu_id_source}"
        BINARY_DIR "${PROJECT_BINARY_DIR}/cpu-id-client-build"
        DOWNLOAD_COMMAND ""
        CMAKE_ARGS "-DDynamoRIO_DIR=${cpu_id_runtime}/cmake"
            "-DCMAKE_BUILD_TYPE=Release" "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND "${CMAKE_COMMAND}" "-DRUNTIME=${cpu_id_runtime}"
            "-DSOURCE=${cpu_id_runtime_source}"
            "-DCLIENT=<BINARY_DIR>/libshadps4_cpu_id.so" "-DOUTPUT=${cpu_id_bundle}"
            -P "${cpu_id_source}/package.cmake"
        DEPENDS shadps4_cpu_id_runtime)
endif()
add_dependencies(shadps4 shadps4_cpu_id_bundle)
install(DIRECTORY "${cpu_id_bundle}" DESTINATION libexec/shadps4 USE_SOURCE_PERMISSIONS)
