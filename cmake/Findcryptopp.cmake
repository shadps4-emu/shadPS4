# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(CRYPTOPP QUIET IMPORTED_TARGET libcryptopp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cryptopp
    REQUIRED_VARS CRYPTOPP_LINK_LIBRARIES
    VERSION_VAR CRYPTOPP_VERSION
)

if (cryptopp_FOUND AND NOT TARGET cryptopp::cryptopp)
    add_library(cryptopp::cryptopp ALIAS PkgConfig::CRYPTOPP)
endif()
