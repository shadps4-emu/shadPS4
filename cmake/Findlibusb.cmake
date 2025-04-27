# SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_package(PkgConfig QUIET)
pkg_search_module(LIBUSB QUIET IMPORTED_TARGET libusb-1.0)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libusb
    REQUIRED_VARS LIBUSB_LINK_LIBRARIES
    VERSION_VAR LIBUSB_VERSION
)

if (libusb_FOUND AND NOT TARGET libusb::usb)
    add_library(libusb::usb ALIAS PkgConfig::LIBUSB)
endif()
