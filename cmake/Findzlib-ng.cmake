# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(ZLIB_NG QUIET IMPORTED_TARGET zlib-ng)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zlib-ng
    REQUIRED_VARS ZLIB_NG_LINK_LIBRARIES
    VERSION_VAR ZLIB_NG_VERSION
)

if (zlib-ng_FOUND AND NOT TARGET zlib-ng::zlib)
    add_library(zlib-ng::zlib ALIAS PkgConfig::ZLIB_NG)
endif()
