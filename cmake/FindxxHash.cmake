# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(XXHASH QUIET IMPORTED_TARGET libxxhash)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(xxHash
    REQUIRED_VARS XXHASH_LINK_LIBRARIES
    VERSION_VAR XXHASH_VERSION
)

if (xxHash_FOUND AND NOT TARGET xxHash::xxhash)
    add_library(xxHash::xxhash ALIAS PkgConfig::XXHASH)
endif()
