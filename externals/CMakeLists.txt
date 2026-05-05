# SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set_directory_properties(PROPERTIES
    EXCLUDE_FROM_ALL ON
    SYSTEM ON
)

# Set CMP0069 policy to "NEW" in order to ensure consistent behavior when building external targets with LTO enabled
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

if (MSVC)
    # Silence "deprecation" warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

# Boost
if (NOT TARGET Boost::headers)
    add_subdirectory(ext-boost)
endif()

# spdlog
set(SPDLOG_NO_EXCEPTIONS ON)
set(SPDLOG_DISABLE_GLOBAL_LOGGER ON)
if (TARGET fmt::fmt)
    set(SPDLOG_FMT_EXTERNAL ON)
endif()
add_subdirectory(spdlog EXCLUDE_FROM_ALL SYSTEM)
target_compile_definitions(spdlog PUBLIC SPDLOG_FUNCTION=__func__)

# fmtlib
if (NOT TARGET fmt::fmt)
    add_subdirectory(fmt)
endif()

# FFmpeg
if (NOT TARGET FFmpeg::ffmpeg)
    add_subdirectory(ffmpeg-core)
    add_library(FFmpeg::ffmpeg ALIAS ffmpeg)
endif()

# LibAtrac9
file(GLOB LIBATRAC9_SOURCES LibAtrac9/C/src/*.c)
add_library(LibAtrac9 STATIC ${LIBATRAC9_SOURCES})
target_include_directories(LibAtrac9 INTERFACE LibAtrac9/C/src)

# zlib
if (NOT TARGET ZLIB::ZLIB)
    set(ZLIB_ENABLE_TESTS OFF)
    set(WITH_GTEST OFF)
    set(WITH_NEW_STRATEGIES ON)
    set(WITH_NATIVE_INSTRUCTIONS ON)
    set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        ZLIB
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng"
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlib)
    # libpng expects this variable to exist after its find_package(ZLIB)
    set(ZLIB_INCLUDE_DIRS "${FETCHCONTENT_BASE_DIR}/zlib-build")
endif()

# SDL3
if (NOT TARGET SDL3::SDL3)
    set(SDL_TEST_LIBRARY OFF)
    set(SDL_PIPEWIRE OFF)
    add_subdirectory(sdl3)
endif()

# vulkan-headers
if (NOT TARGET Vulkan::Headers)
    set(VULKAN_HEADERS_ENABLE_MODULE OFF)
    add_subdirectory(vulkan-headers)
endif()

# VMA
if (NOT TARGET GPUOpen::VulkanMemoryAllocator)
    add_subdirectory(vma)
endif()

# RenderDoc
if (NOT TARGET RenderDoc::API)
    add_library(renderdoc INTERFACE)
    target_include_directories(renderdoc INTERFACE ./renderdoc)
    add_library(RenderDoc::API ALIAS renderdoc)
endif()

# glslang
if (NOT TARGET glslang::glslang)
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "")
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
    set(ENABLE_SPVREMAPPER OFF CACHE BOOL "")
    set(ENABLE_CTEST OFF CACHE BOOL "")
    set(ENABLE_HLSL OFF CACHE BOOL "")
    set(BUILD_EXTERNAL OFF CACHE BOOL "")
    set(ENABLE_OPT OFF CACHE BOOL "")
    add_subdirectory(glslang)
    file(COPY glslang/SPIRV DESTINATION glslang/glslang FILES_MATCHING PATTERN "*.h")
    target_include_directories(glslang INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/glslang")
endif()

# Robin-map
if (NOT TARGET tsl::robin_map)
    add_subdirectory(robin-map)
endif()

# Xbyak
if (NOT TARGET xbyak::xbyak)
    add_subdirectory(xbyak)
endif()

# MagicEnum
if (NOT TARGET magic_enum::magic_enum)
    add_subdirectory(magic_enum)
endif()

# Toml11
if (NOT TARGET toml11::toml11)
    add_subdirectory(toml11)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        get_target_property(_toml11_compile_options toml11 INTERFACE_COMPILE_OPTIONS)
        list(REMOVE_ITEM _toml11_compile_options "/Zc:preprocessor")
        set_target_properties(toml11 PROPERTIES INTERFACE_COMPILE_OPTIONS ${_toml11_compile_options})
      endif()
    endif()
endif()

# xxHash
if (NOT TARGET xxHash::xxhash)
    add_library(xxhash xxhash/xxhash.h xxhash/xxhash.c)
    target_include_directories(xxhash PUBLIC xxhash)
    add_library(xxHash::xxhash ALIAS xxhash)
endif()

# Zydis
if (NOT TARGET Zydis::Zydis)
    option(ZYDIS_BUILD_TOOLS "" OFF)
    option(ZYDIS_BUILD_EXAMPLES "" OFF)
    add_subdirectory(zydis)
endif()

# sirit
add_subdirectory(sirit)
if (WIN32)
    target_compile_options(sirit PRIVATE "-Wno-error=unused-command-line-argument")
endif()

# half
if (NOT TARGET half::half)
    add_library(half INTERFACE)
    target_include_directories(half INTERFACE half/include)
    add_library(half::half ALIAS half)
endif()

# libpng
if (NOT TARGET PNG::PNG)
    set(PNG_SHARED OFF CACHE BOOL "" FORCE)
    set(PNG_STATIC ON CACHE BOOL "" FORCE)
    set(PNG_TESTS OFF CACHE BOOL "" FORCE)
    set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
    set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(libpng)
    add_library(PNG::PNG ALIAS png_static)
endif()

# Dear ImGui
add_library(Dear_ImGui
        dear_imgui/imgui.cpp
        dear_imgui/imgui_demo.cpp
        dear_imgui/imgui_draw.cpp
        dear_imgui/imgui_internal.h
        dear_imgui/imgui_tables.cpp
        dear_imgui/imgui_widgets.cpp
)
target_include_directories(Dear_ImGui INTERFACE dear_imgui/)

# Tracy
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    option(TRACY_ENABLE "" OFF)
else()
    option(TRACY_ENABLE "" ON)
endif()
option(TRACY_NO_CRASH_HANDLER "" ON) # Otherwise texture cache exceptions will be treaten as a crash
option(TRACY_ON_DEMAND "" ON)
option(TRACY_NO_FRAME_IMAGE "" ON)
option(TRACY_FIBERS "" OFF) # For AmdGpu frontend profiling, disabled due to instability
option(TRACY_NO_SYSTEM_TRACING "" ON)
option(TRACY_NO_CALLSTACK "" ON)
option(TRACY_NO_CODE_TRANSFER "" ON)
option(TRACY_NO_SAMPLING "" ON)
option(TRACY_ONLY_LOCALHOST "" ON)
option(TRACY_NO_CONTEXT_SWITCH "" ON)
add_subdirectory(tracy)

# pugixml
if (NOT TARGET pugixml::pugixml)
    option(PUGIXML_NO_EXCEPTIONS "" ON)
    add_subdirectory(pugixml)
endif()

# libusb
if (NOT TARGET libusb::usb)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        # YOU MUST USE NATIVE LIBUSB
        # using anything else will crash instantly, also freebsd will NOT like it
        # no you cant vendor this libusb, its builtin on freebsd
        find_package(libusb)
    else()
        add_subdirectory(libusb)
        add_library(libusb::usb ALIAS usb-1.0)
    endif()
endif()

# Discord RPC
if (ENABLE_DISCORD_RPC)
    add_subdirectory(discord-rpc)
endif()

# GCN Headers
add_subdirectory(gcn)

# stb
if (NOT TARGET stb::headers)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE stb)
    add_library(stb::headers ALIAS stb)
endif()

# hwinfo
set(HWINFO_STATIC ON)
add_subdirectory(hwinfo)

if (APPLE OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    # date
    if (NOT TARGET date::date-tz)
        option(BUILD_TZ_LIB "" ON)
        option(USE_SYSTEM_TZ_DB "" ON)
        add_subdirectory(date)
    endif()
    if (NOT TARGET epoll-shim)
        add_subdirectory(epoll-shim)
    endif()
endif()

# Apple-only dependencies
if (APPLE)
    # MoltenVK
    if (NOT TARGET MoltenVK)
        set(MVK_EXCLUDE_SPIRV_TOOLS ON)
        set(MVK_USE_METAL_PRIVATE_API ON)
        add_subdirectory(MoltenVK)
    endif()
endif()

#windows only
if (WIN32)
add_subdirectory(ext-wepoll)
endif()

add_subdirectory(aacdec)

#nlohmann json 
if (NOT TARGET nlohmann_json::nlohmann_json)
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)
endif()

# miniz
if (NOT TARGET miniz::miniz)
add_subdirectory(miniz)
add_library(miniz::miniz ALIAS miniz)
endif()

# cli11
if (NOT TARGET CLI11::CLI11)
    set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(CLI11)
endif()

# minimp3
add_library(minimp3 INTERFACE)
target_include_directories(minimp3 INTERFACE minimp3)

# ImGuiFileDialog
add_library(ImGuiFileDialog ImGuiFileDialog/ImGuiFileDialog.cpp)
target_compile_definitions(ImGuiFileDialog PRIVATE USE_STD_FILESYSTEM)
target_include_directories(ImGuiFileDialog INTERFACE ImGuiFileDialog/)
target_link_libraries(ImGuiFileDialog PRIVATE Dear_ImGui)

#openal
if (NOT TARGET OpenAL::OpenAL)
set(ALSOFT_ENABLE_MODULES OFF CACHE BOOL "" FORCE)
set(LIBTYPE "STATIC" CACHE STRING "" FORCE)
# Disable everything we don't need
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_CONFIG OFF CACHE BOOL "" FORCE)

# Backends (platform-specific)
if (WIN32)
    set(ALSOFT_BACKEND_WASAPI ON CACHE BOOL "" FORCE)
    set(ALSOFT_BACKEND_DSOUND OFF CACHE BOOL "" FORCE)
    set(ALSOFT_BACKEND_WINMM OFF CACHE BOOL "" FORCE)
elseif (APPLE)
    set(ALSOFT_BACKEND_COREAUDIO ON CACHE BOOL "" FORCE)
else()
    set(ALSOFT_BACKEND_ALSA ON CACHE BOOL "" FORCE)
    set(ALSOFT_BACKEND_PULSEAUDIO ON CACHE BOOL "" FORCE)
    set(ALSOFT_BACKEND_PIPEWIRE OFF CACHE BOOL "" FORCE)
endif()
# Headless-safe
set(ALSOFT_BACKEND_NULL ON CACHE BOOL "" FORCE)

# Static build
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(openal-soft EXCLUDE_FROM_ALL)
endif()

# cpp-httplib
add_library(Cpp_Httplib INTERFACE)
target_include_directories(Cpp_Httplib INTERFACE cpp-httplib/)

if (TARGET ZLIB::ZLIB)
    target_link_libraries(Cpp_Httplib INTERFACE ZLIB::ZLIB)
    target_compile_definitions(Cpp_Httplib INTERFACE CPPHTTPLIB_ZLIB_SUPPORT)
    message(STATUS "cpp-httplib: zlib support enabled (gzip response decompression)")
else()
    message(STATUS "cpp-httplib: zlib not available — gzip responses will arrive uncompressed")
endif()

if (APPLE AND NOT OPENSSL_ROOT_DIR)
    execute_process(
        COMMAND brew --prefix openssl
        RESULT_VARIABLE BREW_RESULT
        OUTPUT_VARIABLE BREW_OPENSSL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if (BREW_RESULT EQUAL 0)
        set(OPENSSL_ROOT_DIR "${BREW_OPENSSL}")
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
elseif (APPLE AND OPENSSL_ROOT_DIR)
    if (NOT DEFINED OPENSSL_USE_STATIC_LIBS)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
    endif()
    message(STATUS "macOS: using caller-supplied OPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}")
endif()

find_package(OpenSSL QUIET)

if (NOT OPENSSL_FOUND AND MSVC)
    message(STATUS "OpenSSL not found in system, downloading FireDaemon prebuilt for Windows...")
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/external/openssl.zip")
        file(DOWNLOAD https://download.firedaemon.com/FireDaemon-OpenSSL/openssl-3.6.1.zip
             "${CMAKE_BINARY_DIR}/external/openssl.zip" SHOW_PROGRESS
             STATUS OPENSSL_DOWNLOAD_STATUS)
        list(GET OPENSSL_DOWNLOAD_STATUS 0 OPENSSL_DOWNLOAD_CODE)
        if (NOT OPENSSL_DOWNLOAD_CODE EQUAL 0)
            file(REMOVE "${CMAKE_BINARY_DIR}/external/openssl.zip")
            message(WARNING "Failed to download FireDaemon OpenSSL prebuilt; "
                            "libSceHttp HTTPS will fall back to mock at runtime. "
                            "Status: ${OPENSSL_DOWNLOAD_STATUS}")
        endif()
    endif()
    if (EXISTS "${CMAKE_BINARY_DIR}/external/openssl.zip" AND
        NOT EXISTS "${CMAKE_BINARY_DIR}/external/openssl")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/external/openssl")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_BINARY_DIR}/external/openssl.zip"
                        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external/openssl")
    endif()
    if (EXISTS "${CMAKE_BINARY_DIR}/external/openssl/x64")
        set(OPENSSL_ROOT_DIR "${CMAKE_BINARY_DIR}/external/openssl/x64" CACHE STRING "Path to OpenSSL root" FORCE)
        unset(OPENSSL_FOUND CACHE)
        find_package(OpenSSL QUIET)
    endif()
endif()

if (OPENSSL_FOUND)
    target_link_libraries(Cpp_Httplib INTERFACE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(Cpp_Httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
    message(STATUS "cpp-httplib: OpenSSL ${OPENSSL_VERSION} found at ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "cpp-httplib: real HTTPS in libSceHttp enabled")
    if (WIN32 AND DEFINED OPENSSL_ROOT_DIR)
        file(GLOB OPENSSL_DLLS
             "${OPENSSL_ROOT_DIR}/bin/libssl-*.dll"
             "${OPENSSL_ROOT_DIR}/bin/libcrypto-*.dll")
        if (OPENSSL_DLLS)
            set(OPENSSL_RUNTIME_DLLS "${OPENSSL_DLLS}" CACHE INTERNAL
                "OpenSSL DLLs to copy next to shadps4.exe (consumed by top-level CMakeLists)")
            list(LENGTH OPENSSL_DLLS OPENSSL_DLL_COUNT)
            message(STATUS "cpp-httplib: ${OPENSSL_DLL_COUNT} OpenSSL DLL(s) staged in OPENSSL_RUNTIME_DLLS for post-build copy")
        endif()
    endif()
    set(SHADPS4_OPENSSL_FOUND TRUE CACHE INTERNAL "OpenSSL was found and linked into Cpp_Httplib")
else()
    message(STATUS "cpp-httplib: OpenSSL not available — libSceHttp HTTPS will fall back to mock at runtime.")
    set(SHADPS4_OPENSSL_FOUND FALSE CACHE INTERNAL "OpenSSL was not found")
endif()

