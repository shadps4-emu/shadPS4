# SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

# Version 3.24 needed for FetchContent OVERRIDE_FIND_PACKAGE
cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(APPLE)
    list(APPEND ADDITIONAL_LANGUAGES OBJC)
    # Starting with 15.4, Rosetta 2 has support for all the necessary instruction sets.
    set(CMAKE_OSX_DEPLOYMENT_TARGET 15.4 CACHE STRING "")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(shadPS4 CXX C ASM ${ADDITIONAL_LANGUAGES})

# Forcing PIE makes sure that the base address is high enough so that it doesn't clash with the PS4 memory.
if(UNIX AND NOT APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

    # check PIE support at link time
    include(CheckPIESupported)
    check_pie_supported(OUTPUT_VARIABLE pie_check LANGUAGES C CXX)
    if(NOT CMAKE_C_LINK_PIE_SUPPORTED OR NOT CMAKE_CXX_LINK_PIE_SUPPORTED)
        message(WARNING "PIE is not supported at link time: ${pie_check}")
    endif()
endif()

option(ENABLE_DISCORD_RPC "Enable the Discord RPC integration" ON)
option(ENABLE_UPDATER "Enables the options to updater" ON)

# First, determine whether to use CMAKE_OSX_ARCHITECTURES or CMAKE_SYSTEM_PROCESSOR.
if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(BASE_ARCHITECTURE "${CMAKE_OSX_ARCHITECTURES}")
elseif (CMAKE_SYSTEM_PROCESSOR)
    set(BASE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
else()
    set(BASE_ARCHITECTURE "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

# Next, match common architecture strings down to a known common value.
if (BASE_ARCHITECTURE MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(ARCHITECTURE "x86_64")
elseif (BASE_ARCHITECTURE MATCHES "(aarch64)|(AARCH64)|(arm64)|(ARM64)")
    set(ARCHITECTURE "arm64")
else()
    message(FATAL_ERROR "Unsupported CPU architecture: ${BASE_ARCHITECTURE}")
endif()

if (ARCHITECTURE STREQUAL "x86_64")
    # Target x86-64-v3 CPU architecture as this is a good balance between supporting performance critical
    # instructions like AVX2 and maintaining support for older CPUs.
    add_compile_options(-march=x86-64-v3)
endif()

if (APPLE AND ARCHITECTURE STREQUAL "x86_64" AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    # Exclude ARM homebrew path to avoid conflicts when cross compiling.
    list(APPEND CMAKE_IGNORE_PREFIX_PATH "/opt/homebrew")

    # Need to reconfigure pkg-config to use the right architecture library paths.
    # It's not ideal to override these but otherwise the build breaks just by having pkg-config installed.
    set(ENV{PKG_CONFIG_DIR} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig:${CMAKE_SYSROOT}/usr/local/lib/pkgconfig:${CMAKE_SYSROOT}/usr/local/share/pkgconfig")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
endif()

# This function should be passed a list of all files in a target. It will automatically generate file groups
# following the directory hierarchy, so that the layout of the files in IDEs matches the one in the filesystem.
function(create_target_directory_groups target_name)

    # Place any files that aren't in the source list in a separate group so that they don't get in the way.
    source_group("Other Files" REGULAR_EXPRESSION ".")

    get_target_property(target_sources "${target_name}" SOURCES)

    foreach(file_name IN LISTS target_sources)
        get_filename_component(dir_name "${file_name}" PATH)
        # Group names use '\' as a separator even though the entire rest of CMake uses '/'...
        string(REPLACE "/" "\\" group_name "${dir_name}")
        source_group("${group_name}" FILES "${file_name}")
    endforeach()
endfunction()

# Setup a custom clang-format target (if clang-format can be found) that will run
# against all the src files. This should be used before making a pull request.
if (CLANG_FORMAT)
    set(SRCS ${PROJECT_SOURCE_DIR}/src)
    set(CCOMMENT "Running clang format against all the .h and .cpp files in src/")
    if (WIN32)
        add_custom_target(clang-format
            COMMAND powershell.exe -Command "Get-ChildItem '${SRCS}/*' -Include *.cpp,*.h,*.mm -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
            COMMENT ${CCOMMENT})
    else()
        add_custom_target(clang-format
            COMMAND find ${SRCS} -iname *.h -o -iname *.cpp -o -iname *.mm | xargs ${CLANG_FORMAT} -i
            COMMENT ${CCOMMENT})
    endif()
    unset(SRCS)
    unset(CCOMMENT)
endif()

# generate git revision information
include("${CMAKE_CURRENT_SOURCE_DIR}/externals/cmake-modules/GetGitRevisionDescription.cmake")
get_git_head_revision(GIT_REF_SPEC GIT_REV)
git_describe(GIT_DESC --always --long --dirty)
git_branch_name(GIT_BRANCH)
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

message("start git things")

# Try to get the upstream remote and branch
message("check for remote and branch")
execute_process(
  COMMAND git rev-parse --abbrev-ref --symbolic-full-name @{u}
  OUTPUT_VARIABLE GIT_REMOTE_NAME
  RESULT_VARIABLE GIT_REMOTE_RESULT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# If there's no upstream set or the command failed, check remote.pushDefault
if (GIT_REMOTE_RESULT OR GIT_REMOTE_NAME STREQUAL "")
  message(STATUS "check default push")
  execute_process(
    COMMAND git config --get remote.pushDefault
    OUTPUT_VARIABLE GIT_REMOTE_NAME
    RESULT_VARIABLE GIT_REMOTE_RESULT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message(STATUS "got remote: ${GIT_REMOTE_NAME}")
endif()

# If running in GitHub Actions and the above fails
if (GIT_REMOTE_RESULT OR GIT_REMOTE_NAME STREQUAL "")
  message(STATUS "check github")
  set(GIT_REMOTE_NAME "origin")

  # Retrieve environment variables
  if (DEFINED ENV{GITHUB_HEAD_REF} AND NOT "$ENV{GITHUB_HEAD_REF}" STREQUAL "")
    message(STATUS "github head ref: $ENV{GITHUB_HEAD_REF}")
    set(GITHUB_HEAD_REF "$ENV{GITHUB_HEAD_REF}")
  else()
    set(GITHUB_HEAD_REF "")
  endif()

  if (DEFINED ENV{GITHUB_REF} AND NOT "$ENV{GITHUB_REF}" STREQUAL "")
    message(STATUS "github ref: $ENV{GITHUB_REF}")
    string(REGEX REPLACE "^refs/[^/]*/" "" GITHUB_BRANCH "$ENV{GITHUB_REF}")
    string(REGEX MATCH "refs/pull/([0-9]+)/merge" MATCHED_REF "$ENV{GITHUB_REF}")
    if (MATCHED_REF)
      set(PR_NUMBER "${CMAKE_MATCH_1}")
      set(GITHUB_BRANCH "")
      message(STATUS "PR number: ${PR_NUMBER}")
    else()
      set(PR_NUMBER "")
    endif()
  else()
    set(GITHUB_BRANCH "")
    set(PR_NUMBER "")
  endif()

  if (NOT "${PR_NUMBER}" STREQUAL "" AND NOT "${GITHUB_HEAD_REF}" STREQUAL "")
    set(GIT_BRANCH "pr-${PR_NUMBER}-${GITHUB_HEAD_REF}")
  elseif (NOT "${PR_NUMBER}" STREQUAL "" AND NOT "${GITHUB_BRANCH}" STREQUAL "")
    set(GIT_BRANCH "pr-${PR_NUMBER}-${GITHUB_BRANCH}")
  elseif (NOT "${PR_NUMBER}" STREQUAL "")
    set(GIT_BRANCH "pr-${PR_NUMBER}")
  elseif ("${PR_NUMBER}" STREQUAL "" AND NOT "${GITHUB_HEAD_REF}" STREQUAL "")
    set(GIT_BRANCH "${GITHUB_HEAD_REF}")
  elseif ("${PR_NUMBER}" STREQUAL "" AND NOT "${GITHUB_BRANCH}" STREQUAL "")
    set(GIT_BRANCH "${GITHUB_BRANCH}")
  elseif ("${PR_NUMBER}" STREQUAL "" AND NOT "${GITHUB_REF}" STREQUAL "")
    set(GIT_BRANCH "${GITHUB_REF}")
  elseif("${GIT_BRANCH}" STREQUAL "")
    message(STATUS "couldn't find branch")
    set(GIT_BRANCH "detached-head")
  endif()
else()
  # Extract remote name if the output contains a remote/branch format
  string(FIND "${GIT_REMOTE_NAME}" "/" INDEX)
  if (INDEX GREATER -1)
    string(SUBSTRING "${GIT_REMOTE_NAME}" 0 "${INDEX}" GIT_REMOTE_NAME)
  elseif("${GIT_REMOTE_NAME}" STREQUAL "")
    message(STATUS "reset to origin")
    set(GIT_REMOTE_NAME "origin")
  endif()
endif()

# Get remote link
message(STATUS "getting remote link")
execute_process(
  COMMAND git config --get remote.${GIT_REMOTE_NAME}.url
  OUTPUT_VARIABLE GIT_REMOTE_URL
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set Version
set(EMULATOR_VERSION_MAJOR "0")
set(EMULATOR_VERSION_MINOR "14")
set(EMULATOR_VERSION_PATCH "1")

set_source_files_properties(src/shadps4.rc PROPERTIES COMPILE_DEFINITIONS "EMULATOR_VERSION_MAJOR=${EMULATOR_VERSION_MAJOR};EMULATOR_VERSION_MINOR=${EMULATOR_VERSION_MINOR};EMULATOR_VERSION_PATCH=${EMULATOR_VERSION_PATCH}")

set(APP_VERSION "${EMULATOR_VERSION_MAJOR}.${EMULATOR_VERSION_MINOR}.${EMULATOR_VERSION_PATCH} WIP")
set(APP_IS_RELEASE false)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/scm_rev.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/common/scm_rev.cpp" @ONLY)

message("-- end git things, remote: ${GIT_REMOTE_NAME}, branch: ${GIT_BRANCH}, link: ${GIT_REMOTE_URL}")

string(TOLOWER "${GIT_REMOTE_URL}" GIT_REMOTE_URL_LOWER)
if(NOT (GIT_REMOTE_URL_LOWER MATCHES "shadps4-emu/shadps4" AND (GIT_BRANCH STREQUAL "main" OR "$ENV{GITHUB_REF}" MATCHES "refs/tags/")))
    message(STATUS "not main, disabling auto update")
    set(ENABLE_UPDATER OFF)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Boost 1.84.0 CONFIG)
find_package(CLI11 2.6.1 CONFIG)
find_package(FFmpeg 5.1.2 MODULE)
find_package(fmt 10.2.0 CONFIG)
find_package(glslang 15 CONFIG)
find_package(half 1.12.0 MODULE)
find_package(magic_enum 0.9.7 CONFIG)
find_package(PNG 1.6 MODULE)
find_package(RenderDoc 1.6.0 MODULE)
find_package(SDL3_mixer 2.8.1 CONFIG)
if (SDL3_mixer_FOUND)
    find_package(SDL3 3.1.2 CONFIG)
endif()
find_package(stb MODULE)
find_package(toml11 4.2.0 CONFIG)
find_package(tsl-robin-map 1.3.0 CONFIG)
find_package(VulkanHeaders 1.4.329 CONFIG)
find_package(VulkanMemoryAllocator 3.1.0 CONFIG)
find_package(xbyak 7.07 CONFIG)
find_package(xxHash 0.8.2 MODULE)
find_package(ZLIB 1.3 MODULE)
find_package(Zydis 5.0.0 CONFIG)
find_package(pugixml 1.14 CONFIG)
if (APPLE)
    find_package(date 3.0.1 CONFIG)
    find_package(epoll-shim 3.14 CONFIG)
endif()
list(POP_BACK CMAKE_MODULE_PATH)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    # libc++ requires -fexperimental-library to enable std::jthread and std::stop_token support.
    include(CheckCXXSymbolExists)
    check_cxx_symbol_exists(_LIBCPP_VERSION version LIBCPP)
    if(LIBCPP)
        add_compile_options(-fexperimental-library)
    endif()
endif()

add_subdirectory(externals)
include_directories(src)

set(AJM_LIB src/core/libraries/ajm/ajm.cpp
            src/core/libraries/ajm/ajm.h
            src/core/libraries/ajm/ajm_aac.cpp
            src/core/libraries/ajm/ajm_aac.h
            src/core/libraries/ajm/ajm_at9.cpp
            src/core/libraries/ajm/ajm_at9.h
            src/core/libraries/ajm/ajm_batch.cpp
            src/core/libraries/ajm/ajm_batch.h
            src/core/libraries/ajm/ajm_context.cpp
            src/core/libraries/ajm/ajm_context.h
            src/core/libraries/ajm/ajm_error.h
            src/core/libraries/ajm/ajm_instance_statistics.cpp
            src/core/libraries/ajm/ajm_instance_statistics.h
            src/core/libraries/ajm/ajm_instance.cpp
            src/core/libraries/ajm/ajm_instance.h
            src/core/libraries/ajm/ajm_mp3.cpp
            src/core/libraries/ajm/ajm_mp3.h
)

set(AUDIO_LIB src/core/libraries/audio/audioin.cpp
              src/core/libraries/audio/audioin.h
              src/core/libraries/audio/audioin_backend.h
              src/core/libraries/audio/audioin_error.h
              src/core/libraries/audio/sdl_audio_in.cpp
              src/core/libraries/voice/voice.cpp
              src/core/libraries/voice/voice.h
              src/core/libraries/audio/audioout.cpp
              src/core/libraries/audio/audioout.h
              src/core/libraries/audio/audioout_backend.h
              src/core/libraries/audio/audioout_error.h
              src/core/libraries/audio/sdl_audio_out.cpp
              src/core/libraries/ngs2/ngs2.cpp
              src/core/libraries/ngs2/ngs2.h
)

set(GNM_LIB src/core/libraries/gnmdriver/gnmdriver.cpp
            src/core/libraries/gnmdriver/gnmdriver.h
            src/core/libraries/gnmdriver/gnmdriver_init.h
            src/core/libraries/gnmdriver/gnm_error.h
)

set(KERNEL_LIB src/core/libraries/kernel/sync/mutex.cpp
               src/core/libraries/kernel/sync/mutex.h
               src/core/libraries/kernel/sync/semaphore.h
               src/core/libraries/kernel/threads/condvar.cpp
               src/core/libraries/kernel/threads/event_flag.cpp
               src/core/libraries/kernel/threads/exception.cpp
               src/core/libraries/kernel/threads/exception.h
               src/core/libraries/kernel/threads/mutex.cpp
               src/core/libraries/kernel/threads/pthread_attr.cpp
               src/core/libraries/kernel/threads/pthread_clean.cpp
               src/core/libraries/kernel/threads/pthread.cpp
               src/core/libraries/kernel/threads/pthread_spec.cpp
               src/core/libraries/kernel/threads/rwlock.cpp
               src/core/libraries/kernel/threads/semaphore.cpp
               src/core/libraries/kernel/threads/sleepq.cpp
               src/core/libraries/kernel/threads/sleepq.h
               src/core/libraries/kernel/threads/stack.cpp
               src/core/libraries/kernel/threads/tcb.cpp
               src/core/libraries/kernel/threads/pthread.h
               src/core/libraries/kernel/threads/thread_state.cpp
               src/core/libraries/kernel/threads/thread_state.h
               src/core/libraries/kernel/process.cpp
               src/core/libraries/kernel/process.h
               src/core/libraries/kernel/debug.cpp
               src/core/libraries/kernel/debug.h
               src/core/libraries/kernel/equeue.cpp
               src/core/libraries/kernel/equeue.h
               src/core/libraries/kernel/file_system.cpp
               src/core/libraries/kernel/file_system.h
               src/core/libraries/kernel/kernel.cpp
               src/core/libraries/kernel/kernel.h
               src/core/libraries/kernel/memory.cpp
               src/core/libraries/kernel/memory.h
               src/core/libraries/kernel/threads.cpp
               src/core/libraries/kernel/threads.h
               src/core/libraries/kernel/time.cpp
               src/core/libraries/kernel/time.h
               src/core/libraries/kernel/orbis_error.h
               src/core/libraries/kernel/posix_error.h
               src/core/libraries/kernel/aio.cpp
               src/core/libraries/kernel/aio.h
)

set(NETWORK_LIBS src/core/libraries/network/http.cpp
                 src/core/libraries/network/http.h
                 src/core/libraries/network/http_error.h
                 src/core/libraries/network/http2.cpp
                 src/core/libraries/network/http2.h
                 src/core/libraries/network/net.cpp
                 src/core/libraries/network/netctl.cpp
                 src/core/libraries/network/netctl.h
                 src/core/libraries/network/net_ctl_obj.cpp
                 src/core/libraries/network/net_ctl_obj.h
                 src/core/libraries/network/net_ctl_codes.h
                 src/core/libraries/network/net_util.cpp
                 src/core/libraries/network/net_util.h
                 src/core/libraries/network/net_epoll.cpp
                 src/core/libraries/network/net_epoll.h
                 src/core/libraries/network/net_resolver.cpp
                 src/core/libraries/network/net_resolver.h
                 src/core/libraries/network/net_error.h
                 src/core/libraries/network/net.h
                 src/core/libraries/network/ssl.cpp
                 src/core/libraries/network/ssl.h
                 src/core/libraries/network/ssl2.cpp
                 src/core/libraries/network/ssl2.h
                 src/core/libraries/network/sys_net.cpp
                 src/core/libraries/network/sys_net.h
                 src/core/libraries/network/posix_sockets.cpp
                 src/core/libraries/network/p2p_sockets.cpp
                 src/core/libraries/network/unix_sockets.cpp
                 src/core/libraries/network/sockets.h
)

set(AVPLAYER_LIB src/core/libraries/avplayer/avplayer_common.cpp
                 src/core/libraries/avplayer/avplayer_common.h
                 src/core/libraries/avplayer/avplayer_file_streamer.cpp
                 src/core/libraries/avplayer/avplayer_file_streamer.h
                 src/core/libraries/avplayer/avplayer_impl.cpp
                 src/core/libraries/avplayer/avplayer_impl.h
                 src/core/libraries/avplayer/avplayer_source.cpp
                 src/core/libraries/avplayer/avplayer_source.h
                 src/core/libraries/avplayer/avplayer_state.cpp
                 src/core/libraries/avplayer/avplayer_state.h
                 src/core/libraries/avplayer/avplayer.cpp
                 src/core/libraries/avplayer/avplayer.h
                 src/core/libraries/avplayer/avplayer_error.h
)

set(SYSTEM_LIBS src/core/libraries/system/commondialog.cpp
                src/core/libraries/system/commondialog.h
                src/core/libraries/system/msgdialog.cpp
                src/core/libraries/system/msgdialog.h
                src/core/libraries/system/msgdialog_ui.cpp
                src/core/libraries/system/posix.cpp
                src/core/libraries/system/posix.h
                src/core/libraries/save_data/save_backup.cpp
                src/core/libraries/save_data/save_backup.h
                src/core/libraries/save_data/save_instance.cpp
                src/core/libraries/save_data/save_instance.h
                src/core/libraries/save_data/save_memory.cpp
                src/core/libraries/save_data/save_memory.h
                src/core/libraries/save_data/savedata.cpp
                src/core/libraries/save_data/savedata.h
                src/core/libraries/save_data/savedata_error.h
                src/core/libraries/save_data/dialog/savedatadialog.cpp
                src/core/libraries/save_data/dialog/savedatadialog.h
                src/core/libraries/save_data/dialog/savedatadialog_ui.cpp
                src/core/libraries/save_data/dialog/savedatadialog_ui.h
                src/core/libraries/system/sysmodule.cpp
                src/core/libraries/system/sysmodule.h
                src/core/libraries/system/system_error.h
                src/core/libraries/system/systemservice.cpp
                src/core/libraries/system/systemservice.h
                src/core/libraries/system/systemservice_error.h
                src/core/libraries/system/userservice.cpp
                src/core/libraries/system/userservice.h
                src/core/libraries/system/userservice_error.h
                src/core/libraries/app_content/app_content.cpp
                src/core/libraries/app_content/app_content.h
                src/core/libraries/app_content/app_content_error.h
                src/core/libraries/rtc/rtc.cpp
                src/core/libraries/rtc/rtc.h
                src/core/libraries/rtc/rtc_error.h
                src/core/libraries/rudp/rudp.cpp
                src/core/libraries/rudp/rudp.h
                src/core/libraries/disc_map/disc_map.cpp
                src/core/libraries/disc_map/disc_map.h
                src/core/libraries/disc_map/disc_map_codes.h
                src/core/libraries/ngs2/ngs2.cpp
                src/core/libraries/ngs2/ngs2.h
                src/core/libraries/ngs2/ngs2_error.h
                src/core/libraries/ngs2/ngs2_impl.cpp
                src/core/libraries/ngs2/ngs2_impl.h
                src/core/libraries/ngs2/ngs2_custom.cpp
                src/core/libraries/ngs2/ngs2_custom.h
                src/core/libraries/ngs2/ngs2_reverb.cpp
                src/core/libraries/ngs2/ngs2_reverb.h
                src/core/libraries/ngs2/ngs2_geom.cpp
                src/core/libraries/ngs2/ngs2_geom.h
                src/core/libraries/ngs2/ngs2_pan.cpp
                src/core/libraries/ngs2/ngs2_pan.h
                src/core/libraries/ngs2/ngs2_report.cpp
                src/core/libraries/ngs2/ngs2_report.h
                src/core/libraries/ngs2/ngs2_eq.cpp
                src/core/libraries/ngs2/ngs2_eq.h
                src/core/libraries/ngs2/ngs2_mastering.cpp
                src/core/libraries/ngs2/ngs2_mastering.h
                src/core/libraries/ngs2/ngs2_sampler.cpp
                src/core/libraries/ngs2/ngs2_sampler.h
                src/core/libraries/ngs2/ngs2_submixer.cpp
                src/core/libraries/ngs2/ngs2_submixer.h
                src/core/libraries/ajm/ajm_error.h
                src/core/libraries/audio3d/audio3d.cpp
                src/core/libraries/audio3d/audio3d.h
                src/core/libraries/audio3d/audio3d_error.h
                src/core/libraries/game_live_streaming/gamelivestreaming.cpp
                src/core/libraries/game_live_streaming/gamelivestreaming.h
                src/core/libraries/remote_play/remoteplay.cpp
                src/core/libraries/remote_play/remoteplay.h
                src/core/libraries/share_play/shareplay.cpp
                src/core/libraries/share_play/shareplay.h
                src/core/libraries/razor_cpu/razor_cpu.cpp
                src/core/libraries/razor_cpu/razor_cpu.h
                src/core/libraries/mouse/mouse.cpp
                src/core/libraries/mouse/mouse.h
                src/core/libraries/web_browser_dialog/webbrowserdialog.cpp
                src/core/libraries/web_browser_dialog/webbrowserdialog.h
                src/core/libraries/font/font.cpp
                src/core/libraries/font/font.h
                src/core/libraries/font/fontft.cpp
                src/core/libraries/font/fontft.h
                src/core/libraries/font/font_error.h

)

set(VIDEOOUT_LIB src/core/libraries/videoout/buffer.h
                 src/core/libraries/videoout/driver.cpp
                 src/core/libraries/videoout/driver.h
                 src/core/libraries/videoout/video_out.cpp
                 src/core/libraries/videoout/video_out.h
                 src/core/libraries/videoout/videoout_error.h
)

set(HLE_LIBC_INTERNAL_LIB src/core/libraries/libc_internal/libc_internal.cpp
                 src/core/libraries/libc_internal/libc_internal.h
                 src/core/libraries/libc_internal/libc_internal_io.cpp
                 src/core/libraries/libc_internal/libc_internal_io.h
                 src/core/libraries/libc_internal/libc_internal_memory.cpp
                 src/core/libraries/libc_internal/libc_internal_memory.h
                 src/core/libraries/libc_internal/libc_internal_str.cpp
                 src/core/libraries/libc_internal/libc_internal_str.h
                 src/core/libraries/libc_internal/libc_internal_math.cpp
                 src/core/libraries/libc_internal/libc_internal_math.h
                 src/core/libraries/libc_internal/printf.h
)

set(IME_LIB src/core/libraries/ime/error_dialog.cpp
            src/core/libraries/ime/error_dialog.h
            src/core/libraries/ime/ime_common.h
            src/core/libraries/ime/ime_dialog_ui.cpp
            src/core/libraries/ime/ime_dialog_ui.h
            src/core/libraries/ime/ime_dialog.cpp
            src/core/libraries/ime/ime_dialog.h
            src/core/libraries/ime/ime_ui.cpp
            src/core/libraries/ime/ime_ui.h
            src/core/libraries/ime/ime.cpp
            src/core/libraries/ime/ime.h
            src/core/libraries/ime/ime_error.h
)

set(PAD_LIB src/core/libraries/pad/pad.cpp
            src/core/libraries/pad/pad.h
            src/core/libraries/pad/pad_errors.h
)

set(SYSTEM_GESTURE_LIB
            src/core/libraries/system_gesture/system_gesture.cpp
            src/core/libraries/system_gesture/system_gesture.h
)

set(PNG_LIB src/core/libraries/libpng/pngdec.cpp
            src/core/libraries/libpng/pngdec.h
            src/core/libraries/libpng/pngdec_error.h
            src/core/libraries/libpng/pngenc.cpp
            src/core/libraries/libpng/pngenc.h
            src/core/libraries/libpng/pngenc_error.h
)

set(JPEG_LIB src/core/libraries/jpeg/jpeg_error.h
             src/core/libraries/jpeg/jpegenc.cpp
             src/core/libraries/jpeg/jpegenc.h
)

set(PLAYGO_LIB src/core/libraries/playgo/playgo.cpp
               src/core/libraries/playgo/playgo.h
               src/core/libraries/playgo/playgo_dialog.cpp
               src/core/libraries/playgo/playgo_dialog.h
               src/core/libraries/playgo/playgo_types.h
)

set(RANDOM_LIB src/core/libraries/random/random.cpp
               src/core/libraries/random/random.h
               src/core/libraries/random/random_error.h
)

set(USBD_LIB src/core/libraries/usbd/usbd.cpp
             src/core/libraries/usbd/usbd.h
             src/core/libraries/usbd/usb_backend.h
             src/core/libraries/usbd/emulated/dimensions.cpp
             src/core/libraries/usbd/emulated/dimensions.h
             src/core/libraries/usbd/emulated/infinity.cpp
             src/core/libraries/usbd/emulated/infinity.h
             src/core/libraries/usbd/emulated/skylander.cpp
             src/core/libraries/usbd/emulated/skylander.h
)

set(FIBER_LIB src/core/libraries/fiber/fiber_context.s
              src/core/libraries/fiber/fiber.cpp
              src/core/libraries/fiber/fiber.h
              src/core/libraries/fiber/fiber_error.h
)

set_source_files_properties(src/core/libraries/fiber/fiber_context.s PROPERTIES COMPILE_OPTIONS -Wno-unused-command-line-argument)

set(VDEC_LIB src/core/libraries/videodec/videodec2_impl.cpp
             src/core/libraries/videodec/videodec2_impl.h
             src/core/libraries/videodec/videodec2.cpp
             src/core/libraries/videodec/videodec2.h
             src/core/libraries/videodec/videodec2_avc.h
             src/core/libraries/videodec/videodec.cpp
             src/core/libraries/videodec/videodec.h
             src/core/libraries/videodec/videodec_error.h
             src/core/libraries/videodec/videodec_impl.cpp
             src/core/libraries/videodec/videodec_impl.h
)

set(NP_LIBS src/core/libraries/np/np_error.h
            src/core/libraries/np/np_common.cpp
            src/core/libraries/np/np_common.h
            src/core/libraries/np/np_commerce.cpp
            src/core/libraries/np/np_commerce.h
            src/core/libraries/np/np_manager.cpp
            src/core/libraries/np/np_manager.h
            src/core/libraries/np/np_matching2.cpp
            src/core/libraries/np/np_matching2.h
            src/core/libraries/np/np_score.cpp
            src/core/libraries/np/np_score.h
            src/core/libraries/np/np_trophy.cpp
            src/core/libraries/np/np_trophy.h
            src/core/libraries/np/np_tus.cpp
            src/core/libraries/np/np_tus.h
            src/core/libraries/np/trophy_ui.cpp
            src/core/libraries/np/trophy_ui.h
            src/core/libraries/np/np_web_api.cpp
            src/core/libraries/np/np_web_api.h
            src/core/libraries/np/np_web_api_error.h
            src/core/libraries/np/np_web_api_internal.cpp
            src/core/libraries/np/np_web_api_internal.h
            src/core/libraries/np/np_web_api2.cpp
            src/core/libraries/np/np_web_api2.h
            src/core/libraries/np/np_party.cpp
            src/core/libraries/np/np_party.h
            src/core/libraries/np/np_auth.cpp
            src/core/libraries/np/np_auth.h
            src/core/libraries/np/np_profile_dialog.cpp
            src/core/libraries/np/np_profile_dialog.h
            src/core/libraries/np/np_sns_facebook_dialog.cpp
            src/core/libraries/np/np_sns_facebook_dialog.h
            src/core/libraries/np/np_partner.cpp
            src/core/libraries/np/np_partner.h
            src/core/libraries/np/object_manager.h
)

set(ZLIB_LIB src/core/libraries/zlib/zlib.cpp
             src/core/libraries/zlib/zlib_sce.h
             src/core/libraries/zlib/zlib_error.h
)

set(VR_LIBS  src/core/libraries/hmd/hmd.cpp
             src/core/libraries/hmd/hmd_reprojection.cpp
             src/core/libraries/hmd/hmd_distortion.cpp
             src/core/libraries/hmd/hmd.h
             src/core/libraries/hmd/hmd_setup_dialog.cpp
             src/core/libraries/hmd/hmd_setup_dialog.h
             src/core/libraries/vr_tracker/vr_tracker.cpp
             src/core/libraries/vr_tracker/vr_tracker.h
)

set(MISC_LIBS  src/core/libraries/screenshot/screenshot.cpp
               src/core/libraries/screenshot/screenshot.h
               src/core/libraries/move/move.cpp
               src/core/libraries/move/move.h
               src/core/libraries/move/move_error.h
               src/core/libraries/ulobjmgr/ulobjmgr.cpp
               src/core/libraries/ulobjmgr/ulobjmgr.h
               src/core/libraries/signin_dialog/signindialog.cpp
               src/core/libraries/signin_dialog/signindialog.h
)

set(CAMERA_LIBS  src/core/libraries/camera/camera.cpp
                 src/core/libraries/camera/camera.h
                 src/core/libraries/camera/camera_error.h
)

set(COMPANION_LIBS  src/core/libraries/companion/companion_httpd.cpp
                    src/core/libraries/companion/companion_httpd.h
                    src/core/libraries/companion/companion_util.cpp
                    src/core/libraries/companion/companion_util.h
                    src/core/libraries/companion/companion_error.h
)
set(DEV_TOOLS src/core/devtools/layer.cpp
              src/core/devtools/layer.h
              src/core/devtools/layer_extra.cpp
              src/core/devtools/options.cpp
              src/core/devtools/options.h
              src/core/devtools/gcn/gcn_context_regs.cpp
              src/core/devtools/gcn/gcn_op_names.cpp
              src/core/devtools/gcn/gcn_shader_regs.cpp
              src/core/devtools/widget/cmd_list.cpp
              src/core/devtools/widget/cmd_list.h
              src/core/devtools/widget/common.h
              src/core/devtools/widget/frame_dump.cpp
              src/core/devtools/widget/frame_dump.h
              src/core/devtools/widget/frame_graph.cpp
              src/core/devtools/widget/frame_graph.h
              src/core/devtools/widget/imgui_memory_editor.h
              src/core/devtools/widget/memory_map.cpp
              src/core/devtools/widget/memory_map.h
              src/core/devtools/widget/module_list.cpp
              src/core/devtools/widget/module_list.h
              src/core/devtools/widget/reg_popup.cpp
              src/core/devtools/widget/reg_popup.h
              src/core/devtools/widget/reg_view.cpp
              src/core/devtools/widget/reg_view.h
              src/core/devtools/widget/shader_list.cpp
              src/core/devtools/widget/shader_list.h
              src/core/devtools/widget/text_editor.cpp
              src/core/devtools/widget/text_editor.h
)

set(COMMON src/common/logging/backend.cpp
           src/common/logging/backend.h
           src/common/logging/filter.cpp
           src/common/logging/filter.h
           src/common/logging/formatter.h
           src/common/logging/log_entry.h
           src/common/logging/log.h
           src/common/logging/text_formatter.cpp
           src/common/logging/text_formatter.h
           src/common/logging/types.h
           src/common/aes.h
           src/common/alignment.h
           src/common/arch.h
           src/common/assert.cpp
           src/common/assert.h
           src/common/bit_array.h
           src/common/bit_field.h
           src/common/bounded_threadsafe_queue.h
           src/common/concepts.h
           src/common/config.cpp
           src/common/config.h
           src/common/cstring.h
           src/common/debug.h
           src/common/decoder.cpp
           src/common/decoder.h
           src/common/elf_info.h
           src/common/endian.h
           src/common/enum.h
           src/common/io_file.cpp
           src/common/io_file.h
           src/common/lru_cache.h
           src/common/error.cpp
           src/common/error.h
           src/common/fixed_value.h
           src/common/func_traits.h
           src/common/native_clock.cpp
           src/common/native_clock.h
           src/common/path_util.cpp
           src/common/path_util.h
           src/common/object_pool.h
           src/common/polyfill_thread.h
           src/common/range_lock.h
           src/common/rdtsc.cpp
           src/common/rdtsc.h
           src/common/recursive_lock.cpp
           src/common/recursive_lock.h
           src/common/scope_exit.h
           src/common/serdes.h
           src/common/sha1.h
           src/common/shared_first_mutex.h
           src/common/signal_context.h
           src/common/signal_context.cpp
           src/common/singleton.h
           src/common/slab_heap.h
           src/common/slot_vector.h
           src/common/spin_lock.cpp
           src/common/spin_lock.h
           src/common/stb.cpp
           src/common/stb.h
           src/common/string_literal.h
           src/common/string_util.cpp
           src/common/string_util.h
           src/common/thread.cpp
           src/common/thread.h
           src/common/types.h
           src/common/uint128.h
           src/common/unique_function.h
           src/common/va_ctx.h
           src/common/ntapi.h
           src/common/ntapi.cpp
           src/common/number_utils.h
           src/common/number_utils.cpp
           src/common/memory_patcher.h
           src/common/memory_patcher.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/src/common/scm_rev.cpp
           src/common/scm_rev.h
           src/common/key_manager.cpp
           src/common/key_manager.h
)

if (ENABLE_DISCORD_RPC)
    list(APPEND COMMON src/common/discord_rpc_handler.cpp src/common/discord_rpc_handler.h)
endif()

set(CORE src/core/aerolib/stubs.cpp
         src/core/aerolib/stubs.h
         src/core/aerolib/aerolib.cpp
         src/core/aerolib/aerolib.h
         src/core/address_space.cpp
         src/core/address_space.h
         src/core/file_sys/devices/base_device.cpp
         src/core/file_sys/devices/base_device.h
         src/core/file_sys/devices/ioccom.h
         src/core/file_sys/devices/logger.cpp
         src/core/file_sys/devices/logger.h
         src/core/file_sys/devices/nop_device.h
         src/core/file_sys/devices/console_device.cpp
         src/core/file_sys/devices/console_device.h
         src/core/file_sys/devices/deci_tty6_device.cpp
         src/core/file_sys/devices/deci_tty6_device.h
         src/core/file_sys/devices/random_device.cpp
         src/core/file_sys/devices/random_device.h
         src/core/file_sys/devices/rng_device.cpp
         src/core/file_sys/devices/rng_device.h
         src/core/file_sys/devices/urandom_device.cpp
         src/core/file_sys/devices/urandom_device.h
         src/core/file_sys/devices/srandom_device.cpp
         src/core/file_sys/devices/srandom_device.h
         src/core/file_sys/directories/base_directory.cpp
         src/core/file_sys/directories/base_directory.h
         src/core/file_sys/directories/normal_directory.cpp
         src/core/file_sys/directories/normal_directory.h
         src/core/file_sys/directories/pfs_directory.cpp
         src/core/file_sys/directories/pfs_directory.h
         src/core/file_format/pfs.h
         src/core/file_format/psf.cpp
         src/core/file_format/psf.h
         src/core/file_format/playgo_chunk.cpp
         src/core/file_format/playgo_chunk.h
         src/core/file_format/trp.cpp
         src/core/file_format/trp.h
         src/core/file_format/npbind.cpp
         src/core/file_format/npbind.h
         src/core/file_sys/fs.cpp
         src/core/file_sys/fs.h
         src/core/ipc/ipc.cpp
         src/core/ipc/ipc.h
         src/core/loader/dwarf.cpp
         src/core/loader/dwarf.h
         src/core/loader/elf.cpp
         src/core/loader/elf.h
         src/core/loader/symbols_resolver.h
         src/core/loader/symbols_resolver.cpp
         src/core/libraries/libs.h
         src/core/libraries/libs.cpp
         ${AJM_LIB}
         ${AVPLAYER_LIB}
         ${AUDIO_LIB}
         ${GNM_LIB}
         ${KERNEL_LIB}
         ${NETWORK_LIBS}
         ${SYSTEM_LIBS}
         ${HLE_LIBC_INTERNAL_LIB}
         ${PAD_LIB}
         ${SYSTEM_GESTURE_LIB}
         ${VIDEOOUT_LIB}
         ${NP_LIBS}
         ${PNG_LIB}
         ${JPEG_LIB}
         ${PLAYGO_LIB}
         ${RANDOM_LIB}
         ${USBD_LIB}
         ${ZLIB_LIB}
         ${MISC_LIBS}
         ${IME_LIB}
         ${FIBER_LIB}
         ${VDEC_LIB}
         ${VR_LIBS}
         ${CAMERA_LIBS}
         ${COMPANION_LIBS}
         ${DEV_TOOLS}
         src/core/debug_state.cpp
         src/core/debug_state.h
         src/core/debugger.cpp
         src/core/debugger.h
         src/core/linker.cpp
         src/core/linker.h
         src/core/memory.cpp
         src/core/memory.h
         src/core/module.cpp
         src/core/module.h
         src/core/platform.h
         src/core/signals.cpp
         src/core/signals.h
         src/core/thread.cpp
         src/core/thread.h
         src/core/tls.cpp
         src/core/tls.h
         src/core/emulator_state.cpp
         src/core/emulator_state.h
)

if (ARCHITECTURE STREQUAL "x86_64")
    set(CORE ${CORE}
             src/core/cpu_patches.cpp
             src/core/cpu_patches.h)
endif()

set(SHADER_RECOMPILER src/shader_recompiler/profile.h
                      src/shader_recompiler/recompiler.cpp
                      src/shader_recompiler/recompiler.h
                      src/shader_recompiler/resource.h
                      src/shader_recompiler/info.h
                      src/shader_recompiler/params.h
                      src/shader_recompiler/runtime_info.h
                      src/shader_recompiler/specialization.h
                      src/shader_recompiler/backend/bindings.h
                      src/shader_recompiler/backend/spirv/emit_spirv.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv.h
                      src/shader_recompiler/backend/spirv/emit_spirv_atomic.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_barriers.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_bitwise_conversion.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_composite.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_context_get_set.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_convert.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_floating_point.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_image.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_instructions.h
                      src/shader_recompiler/backend/spirv/emit_spirv_integer.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_logical.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_quad_rect.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_quad_rect.h
                      src/shader_recompiler/backend/spirv/emit_spirv_select.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_shared_memory.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_special.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_undefined.cpp
                      src/shader_recompiler/backend/spirv/emit_spirv_warp.cpp
                      src/shader_recompiler/backend/spirv/spirv_emit_context.cpp
                      src/shader_recompiler/backend/spirv/spirv_emit_context.h
                      src/shader_recompiler/frontend/translate/data_share.cpp
                      src/shader_recompiler/frontend/translate/export.cpp
                      src/shader_recompiler/frontend/translate/scalar_alu.cpp
                      src/shader_recompiler/frontend/translate/scalar_flow.cpp
                      src/shader_recompiler/frontend/translate/scalar_memory.cpp
                      src/shader_recompiler/frontend/translate/translate.cpp
                      src/shader_recompiler/frontend/translate/translate.h
                      src/shader_recompiler/frontend/translate/vector_alu.cpp
                      src/shader_recompiler/frontend/translate/vector_interpolation.cpp
                      src/shader_recompiler/frontend/translate/vector_memory.cpp
                      src/shader_recompiler/frontend/control_flow_graph.cpp
                      src/shader_recompiler/frontend/control_flow_graph.h
                      src/shader_recompiler/frontend/copy_shader.cpp
                      src/shader_recompiler/frontend/copy_shader.h
                      src/shader_recompiler/frontend/decode.cpp
                      src/shader_recompiler/frontend/decode.h
                      src/shader_recompiler/frontend/fetch_shader.cpp
                      src/shader_recompiler/frontend/fetch_shader.h
                      src/shader_recompiler/frontend/format.cpp
                      src/shader_recompiler/frontend/instruction.cpp
                      src/shader_recompiler/frontend/instruction.h
                      src/shader_recompiler/frontend/opcodes.h
                      src/shader_recompiler/frontend/structured_control_flow.cpp
                      src/shader_recompiler/frontend/structured_control_flow.h
                      src/shader_recompiler/ir/passes/constant_propagation_pass.cpp
                      src/shader_recompiler/ir/passes/dead_code_elimination_pass.cpp
                      src/shader_recompiler/ir/passes/flatten_extended_userdata_pass.cpp
                      src/shader_recompiler/ir/passes/hull_shader_transform.cpp
                      src/shader_recompiler/ir/passes/identity_removal_pass.cpp
                      src/shader_recompiler/ir/passes/inject_clip_distance_attributes.cpp
                      src/shader_recompiler/ir/passes/ir_passes.h
                      src/shader_recompiler/ir/passes/lower_buffer_format_to_raw.cpp
                      src/shader_recompiler/ir/passes/lower_fp64_to_fp32.cpp
                      src/shader_recompiler/ir/passes/readlane_elimination_pass.cpp
                      src/shader_recompiler/ir/passes/resource_tracking_pass.cpp
                      src/shader_recompiler/ir/passes/ring_access_elimination.cpp
                      src/shader_recompiler/ir/passes/shader_info_collection_pass.cpp
                      src/shader_recompiler/ir/passes/shared_memory_barrier_pass.cpp
                      src/shader_recompiler/ir/passes/shared_memory_simplify_pass.cpp
                      src/shader_recompiler/ir/passes/shared_memory_to_storage_pass.cpp
                      src/shader_recompiler/ir/passes/ssa_rewrite_pass.cpp
                      src/shader_recompiler/ir/abstract_syntax_list.cpp
                      src/shader_recompiler/ir/abstract_syntax_list.h
                      src/shader_recompiler/ir/attribute.cpp
                      src/shader_recompiler/ir/attribute.h
                      src/shader_recompiler/ir/basic_block.cpp
                      src/shader_recompiler/ir/basic_block.h
                      src/shader_recompiler/ir/breadth_first_search.h
                      src/shader_recompiler/ir/condition.h
                      src/shader_recompiler/ir/ir_emitter.cpp
                      src/shader_recompiler/ir/ir_emitter.h
                      src/shader_recompiler/ir/microinstruction.cpp
                      src/shader_recompiler/ir/opcodes.cpp
                      src/shader_recompiler/ir/opcodes.h
                      src/shader_recompiler/ir/opcodes.inc
                      src/shader_recompiler/ir/operand_helper.h
                      src/shader_recompiler/ir/patch.cpp
                      src/shader_recompiler/ir/patch.h
                      src/shader_recompiler/ir/position.h
                      src/shader_recompiler/ir/post_order.cpp
                      src/shader_recompiler/ir/post_order.h
                      src/shader_recompiler/ir/program.cpp
                      src/shader_recompiler/ir/program.h
                      src/shader_recompiler/ir/reinterpret.h
                      src/shader_recompiler/ir/reg.h
                      src/shader_recompiler/ir/type.cpp
                      src/shader_recompiler/ir/type.h
                      src/shader_recompiler/ir/value.cpp
                      src/shader_recompiler/ir/value.h
)

set(VIDEO_CORE src/video_core/amdgpu/cb_db_extent.h
               src/video_core/amdgpu/liverpool.cpp
               src/video_core/amdgpu/liverpool.h
               src/video_core/amdgpu/pixel_format.cpp
               src/video_core/amdgpu/pixel_format.h
               src/video_core/amdgpu/pm4_cmds.h
               src/video_core/amdgpu/pm4_opcodes.h
               src/video_core/amdgpu/regs_color.h
               src/video_core/amdgpu/regs_depth.h
               src/video_core/amdgpu/regs.cpp
               src/video_core/amdgpu/regs.h
               src/video_core/amdgpu/regs_primitive.h
               src/video_core/amdgpu/regs_shader.h
               src/video_core/amdgpu/regs_texture.h
               src/video_core/amdgpu/regs_vertex.h
               src/video_core/amdgpu/resource.h
               src/video_core/amdgpu/tiling.cpp
               src/video_core/amdgpu/tiling.h
               src/video_core/buffer_cache/buffer.cpp
               src/video_core/buffer_cache/buffer.h
               src/video_core/buffer_cache/buffer_cache.cpp
               src/video_core/buffer_cache/buffer_cache.h
               src/video_core/buffer_cache/fault_manager.cpp
               src/video_core/buffer_cache/fault_manager.h
               src/video_core/buffer_cache/memory_tracker.h
               src/video_core/buffer_cache/range_set.h
               src/video_core/buffer_cache/region_definitions.h
               src/video_core/buffer_cache/region_manager.h
               src/video_core/renderer_vulkan/liverpool_to_vk.cpp
               src/video_core/renderer_vulkan/liverpool_to_vk.h
               src/video_core/renderer_vulkan/vk_common.cpp
               src/video_core/renderer_vulkan/vk_common.h
               src/video_core/renderer_vulkan/vk_compute_pipeline.cpp
               src/video_core/renderer_vulkan/vk_compute_pipeline.h
               src/video_core/renderer_vulkan/vk_graphics_pipeline.cpp
               src/video_core/renderer_vulkan/vk_graphics_pipeline.h
               src/video_core/renderer_vulkan/vk_instance.cpp
               src/video_core/renderer_vulkan/vk_instance.h
               src/video_core/renderer_vulkan/vk_master_semaphore.cpp
               src/video_core/renderer_vulkan/vk_master_semaphore.h
               src/video_core/renderer_vulkan/vk_pipeline_cache.cpp
               src/video_core/renderer_vulkan/vk_pipeline_cache.h
               src/video_core/renderer_vulkan/vk_pipeline_common.cpp
               src/video_core/renderer_vulkan/vk_pipeline_common.h
               src/video_core/renderer_vulkan/vk_pipeline_serialization.cpp
               src/video_core/renderer_vulkan/vk_pipeline_serialization.h
               src/video_core/renderer_vulkan/vk_platform.cpp
               src/video_core/renderer_vulkan/vk_platform.h
               src/video_core/renderer_vulkan/vk_presenter.cpp
               src/video_core/renderer_vulkan/vk_presenter.h
               src/video_core/renderer_vulkan/vk_rasterizer.cpp
               src/video_core/renderer_vulkan/vk_rasterizer.h
               src/video_core/renderer_vulkan/vk_resource_pool.cpp
               src/video_core/renderer_vulkan/vk_resource_pool.h
               src/video_core/renderer_vulkan/vk_scheduler.cpp
               src/video_core/renderer_vulkan/vk_scheduler.h
               src/video_core/renderer_vulkan/vk_shader_hle.cpp
               src/video_core/renderer_vulkan/vk_shader_hle.h
               src/video_core/renderer_vulkan/vk_shader_util.cpp
               src/video_core/renderer_vulkan/vk_shader_util.h
               src/video_core/renderer_vulkan/vk_swapchain.cpp
               src/video_core/renderer_vulkan/vk_swapchain.h
               src/video_core/renderer_vulkan/host_passes/fsr_pass.cpp
               src/video_core/renderer_vulkan/host_passes/fsr_pass.h
               src/video_core/renderer_vulkan/host_passes/pp_pass.cpp
               src/video_core/renderer_vulkan/host_passes/pp_pass.h
               src/video_core/texture_cache/blit_helper.cpp
               src/video_core/texture_cache/blit_helper.h
               src/video_core/texture_cache/host_compatibility.cpp
               src/video_core/texture_cache/host_compatibility.h
               src/video_core/texture_cache/image.cpp
               src/video_core/texture_cache/image.h
               src/video_core/texture_cache/image_info.cpp
               src/video_core/texture_cache/image_info.h
               src/video_core/texture_cache/image_view.cpp
               src/video_core/texture_cache/image_view.h
               src/video_core/texture_cache/sampler.cpp
               src/video_core/texture_cache/sampler.h
               src/video_core/texture_cache/texture_cache.cpp
               src/video_core/texture_cache/texture_cache.h
               src/video_core/texture_cache/tile_manager.cpp
               src/video_core/texture_cache/tile_manager.h
               src/video_core/texture_cache/types.h
               src/video_core/cache_storage.cpp
               src/video_core/cache_storage.h
               src/video_core/page_manager.cpp
               src/video_core/page_manager.h
               src/video_core/multi_level_page_table.h
               src/video_core/renderdoc.cpp
               src/video_core/renderdoc.h
)

set(IMGUI src/imgui/imgui_config.h
          src/imgui/imgui_layer.h
          src/imgui/imgui_std.h
          src/imgui/imgui_texture.h
          src/imgui/renderer/imgui_core.cpp
          src/imgui/renderer/imgui_core.h
          src/imgui/renderer/imgui_impl_sdl3.cpp
          src/imgui/renderer/imgui_impl_sdl3.h
          src/imgui/renderer/imgui_impl_vulkan.cpp
          src/imgui/renderer/imgui_impl_vulkan.h
          src/imgui/renderer/texture_manager.cpp
          src/imgui/renderer/texture_manager.h
)

set(INPUT src/input/controller.cpp
          src/input/controller.h
          src/input/input_handler.cpp
          src/input/input_handler.h
          src/input/input_mouse.cpp
          src/input/input_mouse.h
)

set(EMULATOR src/emulator.cpp
             src/emulator.h
             src/sdl_window.h
             src/sdl_window.cpp
)

add_executable(shadps4
    ${AUDIO_CORE}
    ${IMGUI}
    ${INPUT}
    ${COMMON}
    ${CORE}
    ${SHADER_RECOMPILER}
    ${VIDEO_CORE}
    ${EMULATOR}
    src/main.cpp
    src/emulator.cpp
    src/emulator.h
    src/sdl_window.h
    src/sdl_window.cpp
)

create_target_directory_groups(shadps4)

target_link_libraries(shadps4 PRIVATE magic_enum::magic_enum fmt::fmt toml11::toml11 tsl::robin_map xbyak::xbyak Tracy::TracyClient RenderDoc::API FFmpeg::ffmpeg Dear_ImGui gcn half::half ZLIB::ZLIB PNG::PNG)
target_link_libraries(shadps4 PRIVATE Boost::headers GPUOpen::VulkanMemoryAllocator LibAtrac9 sirit Vulkan::Headers xxHash::xxhash Zydis::Zydis glslang::glslang SDL3::SDL3 SDL3_mixer::SDL3_mixer pugixml::pugixml)
target_link_libraries(shadps4 PRIVATE stb::headers libusb::usb lfreist-hwinfo::hwinfo nlohmann_json::nlohmann_json miniz fdk-aac CLI11::CLI11)

target_compile_definitions(shadps4 PRIVATE IMGUI_USER_CONFIG="imgui/imgui_config.h")
target_compile_definitions(Dear_ImGui PRIVATE IMGUI_USER_CONFIG="${PROJECT_SOURCE_DIR}/src/imgui/imgui_config.h")

if (ENABLE_DISCORD_RPC)
    target_compile_definitions(shadps4 PRIVATE ENABLE_DISCORD_RPC)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # Optional due to https://github.com/shadps4-emu/shadPS4/issues/1704
    if (ENABLE_USERFAULTFD)
        target_compile_definitions(shadps4 PRIVATE ENABLE_USERFAULTFD)
    endif()

    target_link_libraries(shadps4 PRIVATE uuid)
endif()

if (APPLE)
    # Include MoltenVK, along with an ICD file so it can be found by the system Vulkan loader if used for loading layers.
    set_property(TARGET shadps4 APPEND PROPERTY BUILD_RPATH "@executable_path")
    set(MVK_DST ${CMAKE_CURRENT_BINARY_DIR})
    set(MVK_DYLIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/externals/MoltenVK/MoltenVK/libMoltenVK.dylib)
    set(MVK_DYLIB_DST ${MVK_DST}/libMoltenVK.dylib)
    set(MVK_ICD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/externals/MoltenVK/MoltenVK/icd/MoltenVK_icd.json)
    set(MVK_ICD_DST ${MVK_DST}/MoltenVK_icd.json)

    add_custom_command(
        OUTPUT ${MVK_ICD_DST}
        DEPENDS ${MVK_ICD_SRC} ${MVK_DST}
        COMMAND ${CMAKE_COMMAND} -E copy ${MVK_ICD_SRC} ${MVK_ICD_DST})
    add_custom_command(
        OUTPUT ${MVK_DYLIB_DST}
        DEPENDS ${MVK_DYLIB_SRC} ${MVK_DST}
        COMMAND ${CMAKE_COMMAND} -E copy ${MVK_DYLIB_SRC} ${MVK_DYLIB_DST})
    add_custom_target(CopyMoltenVK DEPENDS ${MVK_ICD_DST} ${MVK_DYLIB_DST})
    add_dependencies(CopyMoltenVK MoltenVK)
    add_dependencies(shadps4 CopyMoltenVK)

    if (ARCHITECTURE STREQUAL "x86_64")
        # Reserve system-managed memory space.
        target_link_options(shadps4 PRIVATE -Wl,-ld_classic,-no_pie,-no_fixup_chains,-no_huge,-pagezero_size,0x4000,-segaddr,TCB_SPACE,0x4000,-segaddr,SYSTEM_MANAGED,0x400000,-segaddr,SYSTEM_RESERVED,0x7FFFFC000,-segaddr,USER_AREA,0x7000000000,-image_base,0x700000000000)
    endif()

    # Replacement for std::chrono::time_zone
    target_link_libraries(shadps4 PRIVATE date::date-tz epoll-shim)
endif()

if (WIN32)
    target_link_libraries(shadps4 PRIVATE mincore wepoll wbemuuid)

    if (MSVC)
        # MSVC likes putting opinions on what people can use, disable:
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE _SCL_SECURE_NO_WARNINGS)
    endif()

    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

    if (MSVC)
        # Needed for conflicts with time.h of windows.h
        add_compile_definitions(_TIMESPEC_DEFINED)
    endif()

    # Target Windows 10 RS5
    add_compile_definitions(NTDDI_VERSION=0x0A000006 _WIN32_WINNT=0x0A00 WINVER=0x0A00)

    if (MSVC)
        target_link_libraries(shadps4 PRIVATE clang_rt.builtins-x86_64.lib)
    endif()

    # Disable ASLR so we can reserve the user area
    if (MSVC)
        target_link_options(shadps4 PRIVATE /DYNAMICBASE:NO)
    else()
        target_link_options(shadps4 PRIVATE -Wl,--disable-dynamicbase)
    endif()

    # Increase stack commit area (Needed, otherwise there are crashes)
    if (MSVC)
        target_link_options(shadps4 PRIVATE /STACK:0x200000,0x200000)
    else()
        target_link_options(shadps4 PRIVATE -Wl,--stack,2097152)
    endif()

    # Change base image address
    if (MSVC)
        target_link_options(shadps4 PRIVATE /BASE:0x700000000000)
    else()
        target_link_options(shadps4 PRIVATE -Wl,--image-base=0x700000000000)
    endif()
endif()

if (WIN32)
    target_sources(shadps4 PRIVATE src/shadps4.rc)
endif()

add_compile_definitions(BOOST_ASIO_STANDALONE)

target_include_directories(shadps4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Shaders sources
set(HOST_SHADERS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/video_core/host_shaders)

add_subdirectory(${HOST_SHADERS_INCLUDE})
add_dependencies(shadps4 host_shaders)
target_include_directories(shadps4 PRIVATE ${HOST_SHADERS_INCLUDE})

# embed resources

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeRC.cmake")
cmrc_add_resource_library(embedded-resources
        ALIAS res::embedded
        NAMESPACE res
	src/images/trophy.wav
        src/images/bronze.png
        src/images/gold.png
        src/images/platinum.png
        src/images/silver.png)
target_link_libraries(shadps4 PRIVATE res::embedded)

# ImGui resources
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/renderer)
add_dependencies(shadps4 ImGui_Resources)
target_include_directories(shadps4 PRIVATE ${IMGUI_RESOURCES_INCLUDE})


# Discord RPC
if (ENABLE_DISCORD_RPC)
    target_link_libraries(shadps4 PRIVATE discord-rpc)
endif()

# Install rules
install(TARGETS shadps4 BUNDLE DESTINATION .)
