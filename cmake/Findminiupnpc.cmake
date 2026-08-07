# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(MINIUPNPC QUIET IMPORTED_TARGET miniupnpc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(miniupnpc
    REQUIRED_VARS MINIUPNPC_LINK_LIBRARIES
    VERSION_VAR MINIUPNPC_VERSION
)

if (miniupnpc_FOUND AND NOT TARGET miniupnpc::miniupnpc)
    target_include_directories(PkgConfig::MINIUPNPC INTERFACE "${MINIUPNPC_INCLUDEDIR}/miniupnpc")
    add_library(miniupnpc::miniupnpc ALIAS PkgConfig::MINIUPNPC)
endif()
