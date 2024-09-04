# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(APPLE)
    enable_language(OBJC)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project(shadPS4)

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

option(ENABLE_QT_GUI "Enable the Qt GUI. If not selected then the emulator uses a minimal SDL-based UI instead" OFF)

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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# generate git revision information
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/cmake-modules/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REF_SPEC GIT_REV)
git_describe(GIT_DESC --always --long --dirty)
git_branch_name(GIT_BRANCH)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/common/scm_rev.cpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/common/scm_rev.cpp" @ONLY)

find_package(Boost 1.84.0 CONFIG)
find_package(FFmpeg 5.1.2 MODULE)
find_package(fmt 10.2.0 CONFIG)
find_package(glslang 14.2.0 CONFIG)
find_package(magic_enum 0.9.6 CONFIG)
find_package(RenderDoc 1.6.0 MODULE)
find_package(SDL3 3.1.2 CONFIG)
find_package(toml11 4.2.0 CONFIG)
find_package(tsl-robin-map 1.3.0 CONFIG)
find_package(VulkanHeaders 1.3.289 CONFIG)
find_package(VulkanMemoryAllocator 3.1.0 CONFIG)
find_package(xbyak 7.07 CONFIG)
find_package(xxHash 0.8.2 MODULE)
find_package(zlib-ng 2.1.7 MODULE)
find_package(Zydis 5.0.0 CONFIG)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT MSVC)
    find_package(cryptopp 8.9.0 MODULE)
endif()

if (APPLE)
    find_package(date 3.0.1 CONFIG)
endif()

# Note: Windows always has these functions through winpthreads
include(CheckSymbolExists)
check_symbol_exists(pthread_mutex_timedlock "pthread.h" HAVE_PTHREAD_MUTEX_TIMEDLOCK)
if(HAVE_PTHREAD_MUTEX_TIMEDLOCK OR WIN32)
    add_compile_options(-DHAVE_PTHREAD_MUTEX_TIMEDLOCK)
endif()

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

if(ENABLE_QT_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent LinguistTools Network)
    qt_standard_project_setup()
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)

    set(QT_TRANSLATIONS "${PROJECT_SOURCE_DIR}/src/qt_gui/translations")
    file(GLOB_RECURSE TRANSLATIONS_TS ${QT_TRANSLATIONS}/*.ts)

    set_source_files_properties(${TRANSLATIONS_TS} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/translations")
    qt_add_translation(TRANSLATIONS_QM ${TRANSLATIONS_TS})

    set(TRANSLATIONS_QRC ${CMAKE_CURRENT_BINARY_DIR}/translations/translations.qrc)
    file(WRITE ${TRANSLATIONS_QRC} "<RCC><qresource prefix=\"translations\">\n")
    foreach (QM ${TRANSLATIONS_QM})
        get_filename_component(QM_FILE ${QM} NAME)
        file(APPEND ${TRANSLATIONS_QRC} "<file>${QM_FILE}</file>\n")
    endforeach (QM)
    file(APPEND ${TRANSLATIONS_QRC} "</qresource></RCC>")

    qt_add_resources(TRANSLATIONS ${TRANSLATIONS_QRC})
endif()

set(AUDIO_CORE src/audio_core/sdl_audio.cpp
               src/audio_core/sdl_audio.h
)

set(AUDIO_LIB src/core/libraries/audio/audioin.cpp
              src/core/libraries/audio/audioin.h
              src/core/libraries/audio/audioout.cpp
              src/core/libraries/audio/audioout.h
              src/core/libraries/ajm/ajm.cpp
              src/core/libraries/ajm/ajm.h
              src/core/libraries/ngs2/ngs2.cpp
              src/core/libraries/ngs2/ngs2.h
)

set(GNM_LIB src/core/libraries/gnmdriver/gnmdriver.cpp
            src/core/libraries/gnmdriver/gnmdriver.h
            src/core/libraries/gnmdriver/gnm_error.h
)

set(KERNEL_LIB
               src/core/libraries/kernel/event_flag/event_flag.cpp
               src/core/libraries/kernel/event_flag/event_flag.h
               src/core/libraries/kernel/event_flag/event_flag_obj.cpp
               src/core/libraries/kernel/event_flag/event_flag_obj.h
               src/core/libraries/kernel/threads/rwlock.cpp
               src/core/libraries/kernel/threads/semaphore.cpp
               src/core/libraries/kernel/threads/keys.cpp
               src/core/libraries/kernel/threads/threads.h
               src/core/libraries/kernel/cpu_management.cpp
               src/core/libraries/kernel/cpu_management.h
               src/core/libraries/kernel/event_queue.cpp
               src/core/libraries/kernel/event_queue.h
               src/core/libraries/kernel/event_queues.cpp
               src/core/libraries/kernel/event_queues.h
               src/core/libraries/kernel/file_system.cpp
               src/core/libraries/kernel/file_system.h
               src/core/libraries/kernel/libkernel.cpp
               src/core/libraries/kernel/libkernel.h
               src/core/libraries/kernel/memory_management.cpp
               src/core/libraries/kernel/memory_management.h
               src/core/libraries/kernel/thread_management.cpp
               src/core/libraries/kernel/thread_management.h
               src/core/libraries/kernel/time_management.cpp
               src/core/libraries/kernel/time_management.h
)

set(NETWORK_LIBS src/core/libraries/network/http.cpp
                 src/core/libraries/network/http.h
                 src/core/libraries/network/net.cpp
                 src/core/libraries/network/netctl.cpp
                 src/core/libraries/network/netctl.h
                 src/core/libraries/network/net.h
                 src/core/libraries/network/ssl.cpp
                 src/core/libraries/network/ssl.h
)

set(SYSTEM_LIBS src/core/libraries/system/commondialog.cpp
                src/core/libraries/system/commondialog.h
                src/core/libraries/system/msgdialog.cpp
                src/core/libraries/system/msgdialog.h
                src/core/libraries/system/msgdialog_ui.cpp
                src/core/libraries/system/posix.cpp
                src/core/libraries/system/posix.h
                src/core/libraries/save_data/error_codes.h
                src/core/libraries/save_data/savedata.cpp
                src/core/libraries/save_data/savedata.h
                src/core/libraries/system/savedatadialog.cpp
                src/core/libraries/system/savedatadialog.h
                src/core/libraries/system/sysmodule.cpp
                src/core/libraries/system/sysmodule.h
                src/core/libraries/system/systemservice.cpp
                src/core/libraries/system/systemservice.h
                src/core/libraries/system/userservice.cpp
                src/core/libraries/system/userservice.h
                src/core/libraries/app_content/app_content.cpp
                src/core/libraries/app_content/app_content.h
                src/core/libraries/rtc/rtc.cpp
                src/core/libraries/rtc/rtc.h
                src/core/libraries/rtc/rtc_error.h
                src/core/libraries/disc_map/disc_map.cpp
                src/core/libraries/disc_map/disc_map.h
                src/core/libraries/disc_map/disc_map_codes.h
                src/core/libraries/avplayer/avplayer_common.cpp
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
                src/core/libraries/ngs2/ngs2.cpp
                src/core/libraries/ngs2/ngs2.h
                src/core/libraries/ngs2/ngs2_error.h
                src/core/libraries/ngs2/ngs2_impl.cpp
                src/core/libraries/ngs2/ngs2_impl.h
                src/core/libraries/ajm/ajm_error.h
)

set(VIDEOOUT_LIB src/core/libraries/videoout/buffer.h
                 src/core/libraries/videoout/driver.cpp
                 src/core/libraries/videoout/driver.h
                 src/core/libraries/videoout/video_out.cpp
                 src/core/libraries/videoout/video_out.h
)

set(LIBC_SOURCES src/core/libraries/libc_internal/libc_internal.cpp
                 src/core/libraries/libc_internal/libc_internal.h
)

set(DIALOGS_LIB src/core/libraries/dialogs/error_dialog.cpp
                src/core/libraries/dialogs/error_dialog.h
                src/core/libraries/dialogs/ime_dialog.cpp
                src/core/libraries/dialogs/ime_dialog.h
                src/core/libraries/dialogs/error_codes.h
)

set(PAD_LIB src/core/libraries/pad/pad.cpp
            src/core/libraries/pad/pad.h
)

set(PNG_LIB src/core/libraries/libpng/pngdec.cpp
            src/core/libraries/libpng/pngdec.h
)

set(PLAYGO_LIB src/core/libraries/playgo/playgo.cpp
               src/core/libraries/playgo/playgo.h
               src/core/libraries/playgo/playgo_types.h
)

set(RANDOM_LIB src/core/libraries/random/random.cpp
               src/core/libraries/random/random.h
               src/core/libraries/random/random_error.h
)

set(USBD_LIB src/core/libraries/usbd/usbd.cpp
             src/core/libraries/usbd/usbd.h
)

set(NP_LIBS src/core/libraries/np_manager/np_manager.cpp
            src/core/libraries/np_manager/np_manager.h
            src/core/libraries/np_score/np_score.cpp
            src/core/libraries/np_score/np_score.h
            src/core/libraries/np_trophy/np_trophy.cpp
            src/core/libraries/np_trophy/np_trophy.h
)

set(MISC_LIBS  src/core/libraries/screenshot/screenshot.cpp
               src/core/libraries/screenshot/screenshot.h
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
           src/common/alignment.h
           src/common/assert.cpp
           src/common/assert.h
           src/common/bit_field.h
           src/common/bounded_threadsafe_queue.h
           src/common/concepts.h
           src/common/config.cpp
           src/common/config.h
           src/common/debug.h
           src/common/disassembler.cpp
           src/common/disassembler.h
           src/common/endian.h
           src/common/enum.h
           src/common/io_file.cpp
           src/common/io_file.h
           src/common/error.cpp
           src/common/error.h
           src/common/scope_exit.h
           src/common/fixed_value.h
           src/common/func_traits.h
           src/common/native_clock.cpp
           src/common/native_clock.h
           src/common/path_util.cpp
           src/common/path_util.h
           src/common/object_pool.h
           src/common/polyfill_thread.h
           src/common/rdtsc.cpp
           src/common/rdtsc.h
           src/common/singleton.h
           src/common/slot_vector.h
           src/common/string_util.cpp
           src/common/string_util.h
           src/common/thread.cpp
           src/common/thread.h
           src/common/types.h
           src/common/uint128.h
           src/common/unique_function.h
           src/common/version.h
           src/common/ntapi.h
           src/common/ntapi.cpp
           src/common/scm_rev.cpp
           src/common/scm_rev.h
)

set(CORE src/core/aerolib/stubs.cpp
         src/core/aerolib/stubs.h
         src/core/aerolib/aerolib.cpp
         src/core/aerolib/aerolib.h
         src/core/address_space.cpp
         src/core/address_space.h
         src/core/cpu_patches.cpp
         src/core/cpu_patches.h
         src/core/crypto/crypto.cpp
         src/core/crypto/crypto.h
         src/core/crypto/keys.h
         src/core/file_format/pfs.h
         src/core/file_format/pkg.cpp
         src/core/file_format/pkg.h
         src/core/file_format/pkg_type.cpp
         src/core/file_format/pkg_type.h
         src/core/file_format/psf.cpp
         src/core/file_format/psf.h
         src/core/file_format/playgo_chunk.cpp
         src/core/file_format/playgo_chunk.h
         src/core/file_format/trp.cpp
         src/core/file_format/trp.h
         src/core/file_format/splash.h
         src/core/file_format/splash.cpp
         src/core/file_sys/fs.cpp
         src/core/file_sys/fs.h
         src/core/loader.cpp
         src/core/loader.h
         src/core/loader/dwarf.cpp
         src/core/loader/dwarf.h
         src/core/loader/elf.cpp
         src/core/loader/elf.h
         src/core/loader/symbols_resolver.h
         src/core/loader/symbols_resolver.cpp
         src/core/libraries/error_codes.h
         src/core/libraries/libs.h
         src/core/libraries/libs.cpp
         ${AUDIO_LIB}
         ${GNM_LIB}
         ${KERNEL_LIB}
         ${NETWORK_LIBS}
         ${SYSTEM_LIBS}
         ${LIBC_SOURCES}
         ${PAD_LIB}
         ${VIDEOOUT_LIB}
         ${NP_LIBS}
         ${PNG_LIB}
         ${PLAYGO_LIB}
         ${RANDOM_LIB}
         ${USBD_LIB}
         ${MISC_LIBS}
         ${DIALOGS_LIB}
         src/core/linker.cpp
         src/core/linker.h
         src/core/memory.cpp
         src/core/memory.h
         src/core/module.cpp
         src/core/module.h
         src/core/platform.h
         src/core/tls.cpp
         src/core/tls.h
         src/core/virtual_memory.cpp
         src/core/virtual_memory.h
)

set(SHADER_RECOMPILER src/shader_recompiler/exception.h
                      src/shader_recompiler/profile.h
                      src/shader_recompiler/recompiler.cpp
                      src/shader_recompiler/recompiler.h
                      src/shader_recompiler/info.h
                      src/shader_recompiler/params.h
                      src/shader_recompiler/runtime_info.h
                      src/shader_recompiler/specialization.h
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
                      src/shader_recompiler/frontend/translate/scalar_memory.cpp
                      src/shader_recompiler/frontend/translate/translate.cpp
                      src/shader_recompiler/frontend/translate/translate.h
                      src/shader_recompiler/frontend/translate/vector_alu.cpp
                      src/shader_recompiler/frontend/translate/vector_interpolation.cpp
                      src/shader_recompiler/frontend/translate/vector_memory.cpp
                      src/shader_recompiler/frontend/control_flow_graph.cpp
                      src/shader_recompiler/frontend/control_flow_graph.h
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
                      src/shader_recompiler/ir/passes/identity_removal_pass.cpp
                      src/shader_recompiler/ir/passes/ir_passes.h
                      src/shader_recompiler/ir/passes/lower_shared_mem_to_registers.cpp
                      src/shader_recompiler/ir/passes/resource_tracking_pass.cpp
                      src/shader_recompiler/ir/passes/shader_info_collection_pass.cpp
                      src/shader_recompiler/ir/passes/ssa_rewrite_pass.cpp
                      src/shader_recompiler/ir/abstract_syntax_list.h
                      src/shader_recompiler/ir/attribute.cpp
                      src/shader_recompiler/ir/attribute.h
                      src/shader_recompiler/ir/basic_block.cpp
                      src/shader_recompiler/ir/basic_block.h
                      src/shader_recompiler/ir/condition.h
                      src/shader_recompiler/ir/ir_emitter.cpp
                      src/shader_recompiler/ir/ir_emitter.h
                      src/shader_recompiler/ir/microinstruction.cpp
                      src/shader_recompiler/ir/opcodes.cpp
                      src/shader_recompiler/ir/opcodes.h
                      src/shader_recompiler/ir/opcodes.inc
                      src/shader_recompiler/ir/post_order.cpp
                      src/shader_recompiler/ir/post_order.h
                      src/shader_recompiler/ir/program.cpp
                      src/shader_recompiler/ir/program.h
                      src/shader_recompiler/ir/reg.h
                      src/shader_recompiler/ir/type.cpp
                      src/shader_recompiler/ir/type.h
                      src/shader_recompiler/ir/value.cpp
                      src/shader_recompiler/ir/value.h
)

set(VIDEO_CORE src/video_core/amdgpu/liverpool.cpp
               src/video_core/amdgpu/liverpool.h
               src/video_core/amdgpu/pixel_format.cpp
               src/video_core/amdgpu/pixel_format.h
               src/video_core/amdgpu/pm4_cmds.h
               src/video_core/amdgpu/pm4_opcodes.h
               src/video_core/amdgpu/resource.h
               src/video_core/amdgpu/default_context.cpp
               src/video_core/buffer_cache/buffer.cpp
               src/video_core/buffer_cache/buffer.h
               src/video_core/buffer_cache/buffer_cache.cpp
               src/video_core/buffer_cache/buffer_cache.h
               src/video_core/buffer_cache/memory_tracker_base.h
               src/video_core/buffer_cache/range_set.h
               src/video_core/buffer_cache/word_manager.h
               src/video_core/renderer_vulkan/liverpool_to_vk.cpp
               src/video_core/renderer_vulkan/liverpool_to_vk.h
               src/video_core/renderer_vulkan/renderer_vulkan.cpp
               src/video_core/renderer_vulkan/renderer_vulkan.h
               src/video_core/renderer_vulkan/vk_common.cpp
               src/video_core/renderer_vulkan/vk_common.h
               src/video_core/renderer_vulkan/vk_compute_pipeline.cpp
               src/video_core/renderer_vulkan/vk_compute_pipeline.h
               src/video_core/renderer_vulkan/vk_descriptor_update_queue.cpp
               src/video_core/renderer_vulkan/vk_descriptor_update_queue.h
               src/video_core/renderer_vulkan/vk_graphics_pipeline.cpp
               src/video_core/renderer_vulkan/vk_graphics_pipeline.h
               src/video_core/renderer_vulkan/vk_instance.cpp
               src/video_core/renderer_vulkan/vk_instance.h
               src/video_core/renderer_vulkan/vk_master_semaphore.cpp
               src/video_core/renderer_vulkan/vk_master_semaphore.h
               src/video_core/renderer_vulkan/vk_pipeline_cache.cpp
               src/video_core/renderer_vulkan/vk_pipeline_cache.h
               src/video_core/renderer_vulkan/vk_platform.cpp
               src/video_core/renderer_vulkan/vk_platform.h
               src/video_core/renderer_vulkan/vk_rasterizer.cpp
               src/video_core/renderer_vulkan/vk_rasterizer.h
               src/video_core/renderer_vulkan/vk_resource_pool.cpp
               src/video_core/renderer_vulkan/vk_resource_pool.h
               src/video_core/renderer_vulkan/vk_scheduler.cpp
               src/video_core/renderer_vulkan/vk_scheduler.h
               src/video_core/renderer_vulkan/vk_shader_util.cpp
               src/video_core/renderer_vulkan/vk_shader_util.h
               src/video_core/renderer_vulkan/vk_swapchain.cpp
               src/video_core/renderer_vulkan/vk_swapchain.h
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
               src/video_core/texture_cache/host_compatibility.h
               src/video_core/page_manager.cpp
               src/video_core/page_manager.h
               src/video_core/multi_level_page_table.h
               src/video_core/renderdoc.cpp
               src/video_core/renderdoc.h
)

set(IMGUI src/imgui/imgui_config.h
          src/imgui/imgui_layer.h
          src/imgui/imgui_std.h
          src/imgui/layer/video_info.cpp
          src/imgui/layer/video_info.h
          src/imgui/renderer/imgui_core.cpp
          src/imgui/renderer/imgui_core.h
          src/imgui/renderer/imgui_impl_sdl3.cpp
          src/imgui/renderer/imgui_impl_sdl3.h
          src/imgui/renderer/imgui_impl_vulkan.cpp
          src/imgui/renderer/imgui_impl_vulkan.h
)

set(INPUT src/input/controller.cpp
          src/input/controller.h
)

set(EMULATOR src/emulator.cpp
             src/emulator.h
             src/sdl_window.h
             src/sdl_window.cpp
)

# The above is shared in SDL and Qt version (TODO share them all)

if(ENABLE_QT_GUI)
qt_add_resources(RESOURCE_FILES src/shadps4.qrc)

set(QT_GUI src/qt_gui/about_dialog.cpp
           src/qt_gui/about_dialog.h
           src/qt_gui/about_dialog.ui
           src/qt_gui/cheats_patches.cpp
           src/qt_gui/cheats_patches.h
           src/qt_gui/memory_patcher.cpp
           src/qt_gui/memory_patcher.h
           src/qt_gui/main_window_ui.h
           src/qt_gui/main_window.cpp
           src/qt_gui/main_window.h
           src/qt_gui/gui_context_menus.h
           src/qt_gui/game_list_utils.h
           src/qt_gui/game_info.cpp
           src/qt_gui/game_info.h
           src/qt_gui/game_list_frame.cpp
           src/qt_gui/game_list_frame.h
           src/qt_gui/game_grid_frame.cpp
           src/qt_gui/game_grid_frame.h
           src/qt_gui/game_install_dialog.cpp
           src/qt_gui/game_install_dialog.h
           src/qt_gui/pkg_viewer.cpp
           src/qt_gui/pkg_viewer.h
           src/qt_gui/trophy_viewer.cpp
           src/qt_gui/trophy_viewer.h
           src/qt_gui/elf_viewer.cpp
           src/qt_gui/elf_viewer.h
           src/qt_gui/main_window_themes.cpp
           src/qt_gui/main_window_themes.h
           src/qt_gui/settings_dialog.cpp
           src/qt_gui/settings_dialog.h
           src/qt_gui/settings_dialog.ui
           src/qt_gui/main.cpp
           ${EMULATOR}
           ${RESOURCE_FILES}
           ${TRANSLATIONS}
)
endif()

if (ENABLE_QT_GUI)
    qt_add_executable(shadps4
        ${AUDIO_CORE}
        ${IMGUI}
        ${INPUT}
        ${QT_GUI}
        ${COMMON}
        ${CORE}
        ${SHADER_RECOMPILER}
        ${VIDEO_CORE}
        ${EMULATOR}
        src/images/shadPS4.icns
    )
else()
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
endif()

create_target_directory_groups(shadps4)

target_link_libraries(shadps4 PRIVATE magic_enum::magic_enum fmt::fmt toml11::toml11 tsl::robin_map xbyak::xbyak Tracy::TracyClient RenderDoc::API FFmpeg::ffmpeg Dear_ImGui)
target_link_libraries(shadps4 PRIVATE Boost::headers GPUOpen::VulkanMemoryAllocator sirit Vulkan::Headers xxHash::xxhash Zydis::Zydis glslang::SPIRV glslang::glslang SDL3::SDL3)

target_compile_definitions(shadps4 PRIVATE IMGUI_USER_CONFIG="imgui/imgui_config.h")
target_compile_definitions(Dear_ImGui PRIVATE IMGUI_USER_CONFIG="${PROJECT_SOURCE_DIR}/src/imgui/imgui_config.h")

if (APPLE)
  option(USE_SYSTEM_VULKAN_LOADER "Enables using the system Vulkan loader instead of directly linking with MoltenVK. Useful for loading validation layers." OFF)
  if (USE_SYSTEM_VULKAN_LOADER)
      target_compile_definitions(shadps4 PRIVATE USE_SYSTEM_VULKAN_LOADER=1)
  else()
      # Link MoltenVK for Vulkan support
      find_library(MOLTENVK MoltenVK REQUIRED)
      target_link_libraries(shadps4 PRIVATE ${MOLTENVK})
  endif()

  # Reserve system-managed memory space.
  target_link_options(shadps4 PRIVATE -Wl,-no_pie,-no_fixup_chains,-no_huge,-pagezero_size,0x4000,-segaddr,TCB_SPACE,0x4000,-segaddr,GUEST_SYSTEM,0x400000,-image_base,0x20000000000)

  # Replacement for std::chrono::time_zone
  target_link_libraries(shadps4 PRIVATE date::date-tz)

  # Half float conversions for F16C patches
  target_link_libraries(shadps4 PRIVATE half)
endif()

if (NOT ENABLE_QT_GUI)
  target_link_libraries(shadps4 PRIVATE SDL3::SDL3)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND MSVC)
    target_link_libraries(shadps4 PRIVATE cryptoppwin zlib-ng::zlib)
else()
    target_link_libraries(shadps4 PRIVATE cryptopp::cryptopp zlib-ng::zlib)
endif()

if (ENABLE_QT_GUI)
   target_link_libraries(shadps4 PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::Network)
   add_definitions(-DENABLE_QT_GUI)
endif()

if (WIN32)
    target_link_libraries(shadps4 PRIVATE mincore winpthreads)

    if (MSVC)
        # MSVC likes putting opinions on what people can use, disable:
        add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
    endif()

    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)

    if (MSVC)
        # Needed for conflicts with time.h of windows.h
        add_definitions(-D_TIMESPEC_DEFINED)
    endif()

    # Target Windows 10 RS5
    add_definitions(-DNTDDI_VERSION=0x0A000006 -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00)

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
endif()

if (WIN32)
    target_sources(shadps4 PRIVATE src/shadps4.rc)
endif()

add_definitions(-DBOOST_ASIO_STANDALONE)

target_include_directories(shadps4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Shaders sources
set(HOST_SHADERS_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/src/video_core/host_shaders)

add_subdirectory(${HOST_SHADERS_INCLUDE})
add_dependencies(shadps4 host_shaders)
target_include_directories(shadps4 PRIVATE ${HOST_SHADERS_INCLUDE})

if (ENABLE_QT_GUI)
    set_target_properties(shadps4 PROPERTIES
#       WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_ICON_FILE shadPS4.icns)

    set_source_files_properties(src/images/shadPS4.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
endif()

if (UNIX AND NOT APPLE)
    if (ENABLE_QT_GUI)
        find_package(OpenSSL REQUIRED)
        target_link_libraries(shadps4 PRIVATE ${OPENSSL_LIBRARIES})
    endif()
endif()
