# SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    file(GLOB QT_KITS LIST_DIRECTORIES true "C:/Qt/*/msvc*")
    list(SORT QT_KITS COMPARE NATURAL)
    list(REVERSE QT_KITS)
    list(GET QT_KITS 0 QT_PREFIX)
    if(QT_PREFIX)
        set(CMAKE_PREFIX_PATH "${QT_PREFIX}" CACHE PATH "Qt prefix auto‑detected" FORCE)
        message(STATUS "Auto-detected Qt prefix: ${QT_PREFIX}")
    else()
        message(WARNING "findQt.cmake: no Qt‑Directory found in C:/Qt – please set CMAKE_PREFIX_PATH manually")
    endif()
endif()
