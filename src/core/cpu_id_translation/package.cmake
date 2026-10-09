# SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

file(MAKE_DIRECTORY "${OUTPUT}")
foreach(relative IN ITEMS bin64/drrun lib64/release/libdynamorio.so
        lib64/release/libdrpreload.so ext/lib64/release/libdrmgr.so ext/lib64/release/libdrwrap.so)
    if (NOT EXISTS "${RUNTIME}/${relative}")
        message(FATAL_ERROR "CPU identity runtime is incomplete: ${RUNTIME}/${relative}")
    endif()
    get_filename_component(directory "${relative}" DIRECTORY)
    file(COPY "${RUNTIME}/${relative}" DESTINATION "${OUTPUT}/${directory}")
endforeach()
file(COPY "${CLIENT}" DESTINATION "${OUTPUT}")
file(COPY "${SOURCE}/License.txt" "${SOURCE}/ACKNOWLEDGEMENTS" DESTINATION "${OUTPUT}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/dynamorio.patch.license" DESTINATION "${OUTPUT}")
