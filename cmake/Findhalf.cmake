# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_path(half_INCLUDE_DIR NAMES half.hpp PATH_SUFFIXES half)

if (half_INCLUDE_DIR)
    file(STRINGS "${half_INCLUDE_DIR}/half.hpp" _ver_line
        REGEX "^// Version [0-9.]+$"
        LIMIT_COUNT 1
    )
    string(REGEX MATCH "[0-9.]+" half_VERSION "${_ver_line}")
    unset(_ver_line)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(half
    REQUIRED_VARS half_INCLUDE_DIR
    VERSION_VAR half_VERSION
)

if (half_FOUND AND NOT TARGET half::half)
    add_library(half::half INTERFACE IMPORTED)
    set_target_properties(half::half PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${half_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(half_INCLUDE_DIR)
