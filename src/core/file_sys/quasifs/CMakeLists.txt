# QuasiFS
add_library(quasifs_lib
    src/quasifs.cpp
    src/quasifs_vdriver.cpp
    src/quasifs_inode_device.cpp
    src/quasifs_inode_directory.cpp
    src/quasifs_inode_regularfile.cpp
    src/quasifs_inode_symlink.cpp
    src/quasifs_partition.cpp    
    )

target_include_directories(quasifs_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(quasifs_lib PRIVATE host_io_lib)
