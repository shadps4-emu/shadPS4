# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(ZARCHIVE QUIET IMPORTED_TARGET zarchive)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZArchive
    REQUIRED_VARS ZARCHIVE_LINK_LIBRARIES
    VERSION_VAR ZARCHIVE_VERSION
)

if (ZArchive_FOUND AND NOT TARGET ZArchive::zarchive)
    add_library(ZArchive::zarchive ALIAS PkgConfig::ZARCHIVE)
endif()
