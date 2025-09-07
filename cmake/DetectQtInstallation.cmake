# SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(highest_version "0")
set(CANDIDATE_DRIVES A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)

foreach(drive ${CANDIDATE_DRIVES})
    file(GLOB kits LIST_DIRECTORIES true CONFIGURE_DEPENDS "${drive}:/Qt/*/msvc*_64")
    foreach(kit IN LISTS kits)
        get_filename_component(version_dir "${kit}" DIRECTORY)
        get_filename_component(kit_version "${version_dir}" NAME)

        message(STATUS "DetectQtInstallation.cmake: Detected Qt: ${kit}")

        if (kit_version VERSION_GREATER highest_version)
            set(highest_version "${kit_version}")
            set(QT_PREFIX "${kit}")

        endif()
    endforeach()
endforeach()

if(QT_PREFIX)
    set(CMAKE_PREFIX_PATH "${QT_PREFIX}" CACHE PATH "Qt prefix auto‑detected" FORCE)
    message(STATUS "DetectQtInstallation.cmake: Choose newest Qt: ${QT_PREFIX}")
else()
    message(STATUS "DetectQtInstallation.cmake: No Qt‑Directory found in <drive>:/Qt – please set CMAKE_PREFIX_PATH manually")
endif()
