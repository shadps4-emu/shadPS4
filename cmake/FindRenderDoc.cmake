# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

find_path(RENDERDOC_INCLUDE_DIR renderdoc_app.h)

if (RENDERDOC_INCLUDE_DIR AND EXISTS "${RENDERDOC_INCLUDE_DIR}/renderdoc_app.h")
    file(STRINGS "${RENDERDOC_INCLUDE_DIR}/renderdoc_app.h" RENDERDOC_VERSION_LINE REGEX "typedef struct RENDERDOC_API")
    string(REGEX REPLACE ".*typedef struct RENDERDOC_API_([0-9]+)_([0-9]+)_([0-9]+).*" "\\1.\\2.\\3" RENDERDOC_VERSION "${RENDERDOC_VERSION_LINE}")
    unset(RENDERDOC_VERSION_LINE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RenderDoc
    REQUIRED_VARS RENDERDOC_INCLUDE_DIR
    VERSION_VAR RENDERDOC_VERSION
)

if (RenderDoc_FOUND AND NOT TARGET RenderDoc::API)
    add_library(RenderDoc::API INTERFACE IMPORTED)
    set_target_properties(RenderDoc::API PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${RENDERDOC_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(RENDERDOC_INCLUDE_DIR)
