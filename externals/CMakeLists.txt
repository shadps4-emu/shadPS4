# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
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

# SDL3_mixer
if (NOT TARGET SDL3_mixer::SDL3_mixer)
    set(SDLMIXER_FLAC OFF)
    set(SDLMIXER_OGG OFF)
    set(SDLMIXER_MOD OFF)
    set(SDLMIXER_MIDI OFF)
    set(SDLMIXER_OPUS OFF)
    set(SDLMIXER_WAVPACK OFF)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(sdl3_mixer)
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
    add_subdirectory(pugixml)
endif()

# libusb
if (NOT TARGET libusb::usb)
    add_subdirectory(ext-libusb)
    add_library(libusb::usb ALIAS usb-1.0)
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

# Apple-only dependencies
if (APPLE)
    # date
    if (NOT TARGET date::date-tz)
        option(BUILD_TZ_LIB "" ON)
        option(USE_SYSTEM_TZ_DB "" ON)
        add_subdirectory(date)
    endif()

    # MoltenVK
    if (NOT TARGET MoltenVK)
        set(MVK_EXCLUDE_SPIRV_TOOLS ON)
        set(MVK_USE_METAL_PRIVATE_API ON)
        add_subdirectory(MoltenVK)
    endif()

    if (NOT TARGET epoll-shim)
        add_subdirectory(epoll-shim)
    endif()
endif()

#windows only
if (WIN32)
add_subdirectory(ext-wepoll)
endif()

if (NOT TARGET fdk-aac)
    set(AACDEC_SRC
        fdk-aac/libAACdec/src/FDK_delay.cpp
        fdk-aac/libAACdec/src/aac_ram.cpp
        fdk-aac/libAACdec/src/aac_rom.cpp
        fdk-aac/libAACdec/src/aacdec_drc.cpp
        fdk-aac/libAACdec/src/aacdec_hcr.cpp
        fdk-aac/libAACdec/src/aacdec_hcr_bit.cpp
        fdk-aac/libAACdec/src/aacdec_hcrs.cpp
        fdk-aac/libAACdec/src/aacdec_pns.cpp
        fdk-aac/libAACdec/src/aacdec_tns.cpp
        fdk-aac/libAACdec/src/aacdecoder.cpp
        fdk-aac/libAACdec/src/aacdecoder_lib.cpp
        fdk-aac/libAACdec/src/block.cpp
        fdk-aac/libAACdec/src/channel.cpp
        fdk-aac/libAACdec/src/channelinfo.cpp
        fdk-aac/libAACdec/src/conceal.cpp
        fdk-aac/libAACdec/src/ldfiltbank.cpp
        fdk-aac/libAACdec/src/pulsedata.cpp
        fdk-aac/libAACdec/src/rvlc.cpp
        fdk-aac/libAACdec/src/rvlcbit.cpp
        fdk-aac/libAACdec/src/rvlcconceal.cpp
        fdk-aac/libAACdec/src/stereo.cpp
        fdk-aac/libAACdec/src/usacdec_ace_d4t64.cpp
        fdk-aac/libAACdec/src/usacdec_ace_ltp.cpp
        fdk-aac/libAACdec/src/usacdec_acelp.cpp
        fdk-aac/libAACdec/src/usacdec_fac.cpp
        fdk-aac/libAACdec/src/usacdec_lpc.cpp
        fdk-aac/libAACdec/src/usacdec_lpd.cpp
        fdk-aac/libAACdec/src/usacdec_rom.cpp
        fdk-aac/libAACdec/src/aac_ram.h
        fdk-aac/libAACdec/src/aac_rom.h
        fdk-aac/libAACdec/src/aacdec_drc.h
        fdk-aac/libAACdec/src/aacdec_drc_types.h
        fdk-aac/libAACdec/src/aacdec_hcr.h
        fdk-aac/libAACdec/src/aacdec_hcr_bit.h
        fdk-aac/libAACdec/src/aacdec_hcr_types.h
        fdk-aac/libAACdec/src/aacdec_hcrs.h
        fdk-aac/libAACdec/src/aacdec_pns.h
        fdk-aac/libAACdec/src/aacdec_tns.h
        fdk-aac/libAACdec/src/aacdecoder.h
        fdk-aac/libAACdec/src/block.h
        fdk-aac/libAACdec/src/channel.h
        fdk-aac/libAACdec/src/channelinfo.h
        fdk-aac/libAACdec/src/conceal.h
        fdk-aac/libAACdec/src/conceal_types.h
        fdk-aac/libAACdec/src/FDK_delay.h
        fdk-aac/libAACdec/src/ldfiltbank.h
        fdk-aac/libAACdec/src/overlapadd.h
        fdk-aac/libAACdec/src/pulsedata.h
        fdk-aac/libAACdec/src/rvlc.h
        fdk-aac/libAACdec/src/rvlc_info.h
        fdk-aac/libAACdec/src/rvlcbit.h
        fdk-aac/libAACdec/src/rvlcconceal.h
        fdk-aac/libAACdec/src/stereo.h
        fdk-aac/libAACdec/src/usacdec_ace_d4t64.h
        fdk-aac/libAACdec/src/usacdec_ace_ltp.h
        fdk-aac/libAACdec/src/usacdec_acelp.h
        fdk-aac/libAACdec/src/usacdec_const.h
        fdk-aac/libAACdec/src/usacdec_fac.h
        fdk-aac/libAACdec/src/usacdec_lpc.h
        fdk-aac/libAACdec/src/usacdec_lpd.h
        fdk-aac/libAACdec/src/usacdec_rom.h
    )

    set(FDK_SRC
        fdk-aac/libFDK/src/FDK_bitbuffer.cpp
        fdk-aac/libFDK/src/FDK_core.cpp
        fdk-aac/libFDK/src/FDK_crc.cpp
        fdk-aac/libFDK/src/FDK_decorrelate.cpp
        fdk-aac/libFDK/src/FDK_hybrid.cpp
        fdk-aac/libFDK/src/FDK_lpc.cpp
        fdk-aac/libFDK/src/FDK_matrixCalloc.cpp
        fdk-aac/libFDK/src/FDK_qmf_domain.cpp
        fdk-aac/libFDK/src/FDK_tools_rom.cpp
        fdk-aac/libFDK/src/FDK_trigFcts.cpp
        fdk-aac/libFDK/src/autocorr2nd.cpp
        fdk-aac/libFDK/src/dct.cpp
        fdk-aac/libFDK/src/fft.cpp
        fdk-aac/libFDK/src/fft_rad2.cpp
        fdk-aac/libFDK/src/fixpoint_math.cpp
        fdk-aac/libFDK/src/huff_nodes.cpp
        fdk-aac/libFDK/src/mdct.cpp
        fdk-aac/libFDK/src/nlc_dec.cpp
        fdk-aac/libFDK/src/qmf.cpp
        fdk-aac/libFDK/src/scale.cpp
    )

    set(SYS_SRC
        fdk-aac/libSYS/src/genericStds.cpp
        fdk-aac/libSYS/src/syslib_channelMapDescr.cpp
    )

    set(ARITHCODING_SRC
        fdk-aac/libArithCoding/src/ac_arith_coder.cpp
    )

    set(MPEGTPDEC_SRC
        fdk-aac/libMpegTPDec/src/tpdec_adif.cpp
        fdk-aac/libMpegTPDec/src/tpdec_adts.cpp
        fdk-aac/libMpegTPDec/src/tpdec_asc.cpp
        fdk-aac/libMpegTPDec/src/tpdec_drm.cpp
        fdk-aac/libMpegTPDec/src/tpdec_latm.cpp
        fdk-aac/libMpegTPDec/src/tpdec_lib.cpp
        fdk-aac/libMpegTPDec/src/tp_version.h
        fdk-aac/libMpegTPDec/src/tpdec_adif.h
        fdk-aac/libMpegTPDec/src/tpdec_adts.h
        fdk-aac/libMpegTPDec/src/tpdec_drm.h
        fdk-aac/libMpegTPDec/src/tpdec_latm.h
    )

    set(SBRDEC_SRC
        fdk-aac/libSBRdec/src/HFgen_preFlat.cpp
        fdk-aac/libSBRdec/src/env_calc.cpp
        fdk-aac/libSBRdec/src/env_dec.cpp
        fdk-aac/libSBRdec/src/env_extr.cpp
        fdk-aac/libSBRdec/src/hbe.cpp
        fdk-aac/libSBRdec/src/huff_dec.cpp
        fdk-aac/libSBRdec/src/lpp_tran.cpp
        fdk-aac/libSBRdec/src/psbitdec.cpp
        fdk-aac/libSBRdec/src/psdec.cpp
        fdk-aac/libSBRdec/src/psdec_drm.cpp
        fdk-aac/libSBRdec/src/psdecrom_drm.cpp
        fdk-aac/libSBRdec/src/pvc_dec.cpp
        fdk-aac/libSBRdec/src/sbr_deb.cpp
        fdk-aac/libSBRdec/src/sbr_dec.cpp
        fdk-aac/libSBRdec/src/sbr_ram.cpp
        fdk-aac/libSBRdec/src/sbr_rom.cpp
        fdk-aac/libSBRdec/src/sbrdec_drc.cpp
        fdk-aac/libSBRdec/src/sbrdec_freq_sca.cpp
        fdk-aac/libSBRdec/src/sbrdecoder.cpp
        fdk-aac/libSBRdec/src/env_calc.h
        fdk-aac/libSBRdec/src/env_dec.h
        fdk-aac/libSBRdec/src/env_extr.h
        fdk-aac/libSBRdec/src/hbe.h
        fdk-aac/libSBRdec/src/HFgen_preFlat.h
        fdk-aac/libSBRdec/src/huff_dec.h
        fdk-aac/libSBRdec/src/lpp_tran.h
        fdk-aac/libSBRdec/src/psbitdec.h
        fdk-aac/libSBRdec/src/psdec.h
        fdk-aac/libSBRdec/src/psdec_drm.h
        fdk-aac/libSBRdec/src/pvc_dec.h
        fdk-aac/libSBRdec/src/sbr_deb.h
        fdk-aac/libSBRdec/src/sbr_dec.h
        fdk-aac/libSBRdec/src/sbr_ram.h
        fdk-aac/libSBRdec/src/sbr_rom.h
        fdk-aac/libSBRdec/src/sbrdec_drc.h
        fdk-aac/libSBRdec/src/sbrdec_freq_sca.h
        fdk-aac/libSBRdec/src/transcendent.h
    )

    set(PCMUTILS_SRC
        fdk-aac/libPCMutils/src/limiter.cpp
        fdk-aac/libPCMutils/src/pcm_utils.cpp
        fdk-aac/libPCMutils/src/pcmdmx_lib.cpp
        fdk-aac/libPCMutils/src/version.h
    )

    set(DRCDEC_SRC
        fdk-aac/libDRCdec/src/FDK_drcDecLib.cpp
        fdk-aac/libDRCdec/src/drcDec_gainDecoder.cpp
        fdk-aac/libDRCdec/src/drcDec_reader.cpp
        fdk-aac/libDRCdec/src/drcDec_rom.cpp
        fdk-aac/libDRCdec/src/drcDec_selectionProcess.cpp
        fdk-aac/libDRCdec/src/drcDec_tools.cpp
        fdk-aac/libDRCdec/src/drcGainDec_init.cpp
        fdk-aac/libDRCdec/src/drcGainDec_preprocess.cpp
        fdk-aac/libDRCdec/src/drcGainDec_process.cpp
        fdk-aac/libDRCdec/src/drcDec_gainDecoder.h
        fdk-aac/libDRCdec/src/drcDec_reader.h
        fdk-aac/libDRCdec/src/drcDec_rom.h
        fdk-aac/libDRCdec/src/drcDec_selectionProcess.h
        fdk-aac/libDRCdec/src/drcDec_tools.h
        fdk-aac/libDRCdec/src/drcDec_types.h
        fdk-aac/libDRCdec/src/drcDecoder.h
        fdk-aac/libDRCdec/src/drcGainDec_init.h
        fdk-aac/libDRCdec/src/drcGainDec_preprocess.h
        fdk-aac/libDRCdec/src/drcGainDec_process.h
    )

    set(SACDEC_SRC
        fdk-aac/libSACdec/src/sac_bitdec.cpp
        fdk-aac/libSACdec/src/sac_calcM1andM2.cpp
        fdk-aac/libSACdec/src/sac_dec.cpp
        fdk-aac/libSACdec/src/sac_dec_conceal.cpp
        fdk-aac/libSACdec/src/sac_dec_lib.cpp
        fdk-aac/libSACdec/src/sac_process.cpp
        fdk-aac/libSACdec/src/sac_qmf.cpp
        fdk-aac/libSACdec/src/sac_reshapeBBEnv.cpp
        fdk-aac/libSACdec/src/sac_rom.cpp
        fdk-aac/libSACdec/src/sac_smoothing.cpp
        fdk-aac/libSACdec/src/sac_stp.cpp
        fdk-aac/libSACdec/src/sac_tsd.cpp
        fdk-aac/libSACdec/src/sac_bitdec.h
        fdk-aac/libSACdec/src/sac_calcM1andM2.h
        fdk-aac/libSACdec/src/sac_dec.h
        fdk-aac/libSACdec/src/sac_dec_conceal.h
        fdk-aac/libSACdec/src/sac_dec_interface.h
        fdk-aac/libSACdec/src/sac_dec_ssc_struct.h
        fdk-aac/libSACdec/src/sac_process.h
        fdk-aac/libSACdec/src/sac_qmf.h
        fdk-aac/libSACdec/src/sac_reshapeBBEnv.h
        fdk-aac/libSACdec/src/sac_rom.h
        fdk-aac/libSACdec/src/sac_smoothing.h
        fdk-aac/libSACdec/src/sac_stp.h
        fdk-aac/libSACdec/src/sac_tsd.h
    )

    add_library(fdk-aac
        ${AACDEC_SRC}
        ${FDK_SRC}
        ${SYS_SRC}
        ${ARITHCODING_SRC}
        ${MPEGTPDEC_SRC}
        ${SBRDEC_SRC}
        ${PCMUTILS_SRC}
        ${DRCDEC_SRC}
        ${SACDEC_SRC}
    )

    target_include_directories(fdk-aac
        PUBLIC
            fdk-aac/libAACdec/include
            fdk-aac/libFDK/include
            fdk-aac/libSYS/include
            fdk-aac/libArithCoding/include
            fdk-aac/libMpegTPDec/include
            fdk-aac/libSBRdec/include
            fdk-aac/libPCMutils/include
            fdk-aac/libDRCdec/include
            fdk-aac/libSACdec/include
    )
endif()

#nlohmann json 
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

# miniz
add_subdirectory(miniz)
