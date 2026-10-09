# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)
project(kernel_unlink_regression LANGUAGES CXX)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This focused fixture currently validates Linux host filesystem behavior")
endif()
enable_testing()
option(KERNEL_UNLINK_SANITIZERS "Enable host ASan/UBSan" OFF)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Compile the exact production syscall bodies with a synthetic mount/handle table.
# The IOFile translation unit, host filesystem calls and errno conversion are real.
function(extract name source first last)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
    file(READ "${source}" body)
    string(FIND "${body}" "${first}" begin)
    string(FIND "${body}" "${last}" end)
    if(begin LESS 0 OR end LESS begin)
        message(FATAL_ERROR "Cannot extract production ${name}")
    endif()
    math(EXPR length "${end} - ${begin}")
    string(SUBSTRING "${body}" ${begin} ${length} body)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${name}.inc" "${body}")
endfunction()
extract(unlink "${REPO_ROOT}/src/core/libraries/kernel/file_system.cpp"
    "s32 PS4_SYSV_ABI posix_unlink(" "#ifdef _WIN32\n\ntypedef struct")
extract(errno "${REPO_ROOT}/src/core/libraries/kernel/kernel.cpp"
    "static thread_local s32 g_posix_errno" "struct OrbisKernelUuid")

add_executable(kernel_unlink_test test_unlink.cpp "${REPO_ROOT}/src/common/io_file.cpp")
target_compile_features(kernel_unlink_test PRIVATE cxx_std_23)
target_compile_definitions(kernel_unlink_test PRIVATE FMT_HEADER_ONLY)
target_include_directories(kernel_unlink_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
    "${REPO_ROOT}/src" "${REPO_ROOT}/externals/fmt/include")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kernel_unlink_test PRIVATE -Wall -Wextra -Werror)
    # IOFile's existing sharing flag is only used on Windows.
    set_source_files_properties("${REPO_ROOT}/src/common/io_file.cpp" PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-parameter")
    if(KERNEL_UNLINK_SANITIZERS)
        target_compile_options(kernel_unlink_test PRIVATE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(kernel_unlink_test PRIVATE -fsanitize=address,undefined)
    endif()
endif()
foreach(case missing sce_missing null long_path directory read_only unknown_mount
        success open_file already_removed no_host errno_mapping permission io_errors)
    add_test(NAME kernel_unlink_${case} COMMAND kernel_unlink_test ${case})
endforeach()
