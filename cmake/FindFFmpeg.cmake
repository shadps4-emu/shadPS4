# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET libavcodec libavfilter libavformat libavutil libswresample libswscale)

find_file(FFMPEG_VERSION_FILE libavutil/ffversion.h HINTS "${FFMPEG_libavutil_INCLUDEDIR}")
if (FFMPEG_VERSION_FILE)
    file(STRINGS "${FFMPEG_VERSION_FILE}" FFMPEG_VERSION_LINE REGEX "FFMPEG_VERSION")
    string(REGEX MATCH "[0-9.]+" FFMPEG_VERSION "${FFMPEG_VERSION_LINE}")
    unset(FFMPEG_VERSION_LINE)
    unset(FFMPEG_VERSION_FILE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFMPEG_LINK_LIBRARIES
    VERSION_VAR FFMPEG_VERSION
)

if (FFmpeg_FOUND AND NOT TARGET FFmpeg::ffmpeg)
    add_library(FFmpeg::ffmpeg ALIAS PkgConfig::FFMPEG)
endif()
