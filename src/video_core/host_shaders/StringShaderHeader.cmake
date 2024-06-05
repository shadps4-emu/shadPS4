# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(SOURCE_FILE ${CMAKE_ARGV3})
set(HEADER_FILE ${CMAKE_ARGV4})
set(INPUT_FILE ${CMAKE_ARGV5})

get_filename_component(CONTENTS_NAME ${SOURCE_FILE} NAME)
string(REPLACE "." "_" CONTENTS_NAME ${CONTENTS_NAME})
string(TOUPPER ${CONTENTS_NAME} CONTENTS_NAME)

FILE(READ ${SOURCE_FILE} line_contents)

# Replace double quotes with single quotes,
# as double quotes will be used to wrap the lines
STRING(REGEX REPLACE "\"" "'" line_contents "${line_contents}")

# CMake separates list elements with semicolons, but semicolons
# are used extensively in the shader code.
# Replace with a temporary marker, to be reverted later.
STRING(REGEX REPLACE ";" "{{SEMICOLON}}" line_contents "${line_contents}")

# Make every line an individual element in the CMake list.
STRING(REGEX REPLACE "\n" ";" line_contents "${line_contents}")

# Build the shader string, wrapping each line in double quotes.
foreach(line IN LISTS line_contents)
    string(CONCAT CONTENTS "${CONTENTS}" \"${line}\\n\"\n)
endforeach()

# Revert the original semicolons in the source.
STRING(REGEX REPLACE "{{SEMICOLON}}" ";" CONTENTS "${CONTENTS}")

get_filename_component(OUTPUT_DIR ${HEADER_FILE} DIRECTORY)
make_directory(${OUTPUT_DIR})
configure_file(${INPUT_FILE} ${HEADER_FILE} @ONLY)
