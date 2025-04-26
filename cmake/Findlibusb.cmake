# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h)
find_library(LIBUSB_LIBRARY NAMES usb-1.0.27)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)

if(LIBUSB_FOUND)
  set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
  set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
endif()
