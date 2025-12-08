# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(jit_tests
    test_arm64_codegen.cpp
    test_register_mapping.cpp
    test_block_manager.cpp
    test_execution_engine.cpp
    test_block_linking.cpp
    main.cpp
)

if (ARCHITECTURE STREQUAL "arm64")
    target_sources(jit_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/arm64_codegen.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/arm64_codegen.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/register_mapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/register_mapping.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/block_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/block_manager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/x86_64_translator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/x86_64_translator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/simd_translator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/simd_translator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/calling_convention.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/jit/calling_convention.h
    )
endif()

target_sources(jit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/assert.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/common/decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_logging_stub.cpp
)

target_link_libraries(jit_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    Zydis::Zydis
    fmt::fmt
)

target_include_directories(jit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../externals/zydis/include
)

target_compile_definitions(jit_tests PRIVATE
    ARCH_ARM64
)

# to make ctest work
add_test(NAME JitTests COMMAND jit_tests)

set_tests_properties(JitTests PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
