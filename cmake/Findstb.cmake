# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_path(stb_image_INCLUDE_DIR stb_image.h PATH_SUFFIXES stb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(stb
    REQUIRED_VARS stb_image_INCLUDE_DIR
)

if (stb_FOUND AND NOT TARGET stb::headers)
    add_library(stb::headers INTERFACE IMPORTED)
    set_property(TARGET stb::headers PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES
            "${stb_image_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(stb_image_INCLUDE_DIR)
