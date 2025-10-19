# Host IO
add_library(host_io_lib
    src/host_io_base.cpp
    src/host_io_virtual.cpp
    src/host_io_posix.cpp
    src/host_io_win32.cpp
    )

target_include_directories(host_io_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)