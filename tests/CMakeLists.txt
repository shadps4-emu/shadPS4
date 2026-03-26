# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# Find or download Google Test
include(FetchContent)
    FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
FetchContent_MakeAvailable(googletest)

set(SETTINGS_TEST_SOURCES
    # Under test
    ${CMAKE_SOURCE_DIR}/src/core/emulator_settings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/emulator_state.cpp

    # Minimal common support
    ${CMAKE_SOURCE_DIR}/src/common/path_util.cpp
    ${CMAKE_SOURCE_DIR}/src/common/assert.cpp
    ${CMAKE_SOURCE_DIR}/src/common/error.cpp
    ${CMAKE_SOURCE_DIR}/src/common/string_util.cpp
    ${CMAKE_SOURCE_DIR}/src/common/logging/filter.cpp
    ${CMAKE_SOURCE_DIR}/src/common/logging/text_formatter.cpp

    # Stubs that replace dependencies
    stubs/log_stub.cpp     
    stubs/scm_rev_stub.cpp
    stubs/sdl_stub.cpp

    # Tests
    test_emulator_settings.cpp
)

add_executable(shadps4_settings_test ${SETTINGS_TEST_SOURCES})

target_include_directories(shadps4_settings_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(shadps4_settings_test PRIVATE
    GTest::gtest_main
    fmt::fmt
    nlohmann_json::nlohmann_json
    toml11::toml11
    SDL3::SDL3 
)

target_compile_features(shadps4_settings_test PRIVATE cxx_std_23)

target_compile_definitions(shadps4_settings_test PRIVATE BOOST_ASIO_STANDALONE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    include(CheckCXXSymbolExists)
    check_cxx_symbol_exists(_LIBCPP_VERSION version LIBCPP)
    if (LIBCPP)
        target_compile_options(shadps4_settings_test PRIVATE -fexperimental-library)
    endif()
endif()

if (WIN32)
    target_compile_definitions(shadps4_settings_test PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        NTDDI_VERSION=0x0A000006
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
    )
    if (MSVC)
        target_compile_definitions(shadps4_settings_test PRIVATE
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_DEPRECATE
            _SCL_SECURE_NO_WARNINGS
            _TIMESPEC_DEFINED
        )
    endif()
endif()

include(GoogleTest)
gtest_discover_tests(shadps4_settings_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    PROPERTIES TIMEOUT 60
)
