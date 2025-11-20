# SPDX-FileCopyrightText: 2020 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(SOURCE_FILE ${CMAKE_ARGV3})
set(HEADER_FILE ${CMAKE_ARGV4})
set(INPUT_FILE ${CMAKE_ARGV5})

get_filename_component(CONTENTS_NAME ${SOURCE_FILE} NAME)
string(REPLACE "." "_" CONTENTS_NAME ${CONTENTS_NAME})
string(TOUPPER ${CONTENTS_NAME} CONTENTS_NAME)

# Function to recursively parse #include directives and replace them with file contents
function(parse_includes file_path output_content)
    file(READ ${file_path} file_content)
    # This regex includes \n at the begin to (hackish) avoid including comments
    string(REGEX MATCHALL "\n#include +\"[^\"]+\"" includes "${file_content}")

    set(parsed_content "${file_content}")
    foreach (include_match ${includes})
        string(REGEX MATCH "\"([^\"]+)\"" _ "${include_match}")
        set(include_file ${CMAKE_MATCH_1})
        get_filename_component(include_full_path "${file_path}" DIRECTORY)
        set(include_full_path "${include_full_path}/${include_file}")

        if (NOT EXISTS "${include_full_path}")
            message(FATAL_ERROR "Included file not found: ${include_full_path} from ${file_path}")
        endif ()

        parse_includes("${include_full_path}" sub_content)
        string(REPLACE "${include_match}" "\n${sub_content}" parsed_content "${parsed_content}")
    endforeach ()
    set(${output_content} "${parsed_content}" PARENT_SCOPE)
endfunction()

parse_includes("${SOURCE_FILE}" CONTENTS)

get_filename_component(OUTPUT_DIR ${HEADER_FILE} DIRECTORY)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
configure_file(${INPUT_FILE} ${HEADER_FILE} @ONLY)
