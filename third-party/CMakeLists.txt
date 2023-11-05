if (MSVC)
    # Silence "deprecation" warnings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)
endif()

# Discord-RPC
set(BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(discord-rpc EXCLUDE_FROM_ALL)
target_include_directories(discord-rpc INTERFACE ./discord-rpc/include)

# fmtlib
add_subdirectory(fmt EXCLUDE_FROM_ALL)

# MagicEnum
add_subdirectory(magic_enum EXCLUDE_FROM_ALL)

# Spdlog
set(SPDLOG_WCHAR_FILENAMES ON CACHE BOOL "")
set(SPDLOG_NO_THREAD_ID ON CACHE BOOL "")
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")
add_subdirectory(spdlog EXCLUDE_FROM_ALL)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ./stb)

# SDL3
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# Toml11
add_subdirectory(toml11 EXCLUDE_FROM_ALL)

# Vulkan
add_subdirectory(vulkan EXCLUDE_FROM_ALL)
target_include_directories(vulkan-1 INTERFACE vulkan/include)

# Winpthreads
if (WIN32)
    add_subdirectory(winpthread EXCLUDE_FROM_ALL)
    target_include_directories(winpthread INTERFACE winpthread/include)
endif()

# xxHash
add_library(xxhash INTERFACE)
target_include_directories(xxhash INTERFACE xxhash)

# Zydis
option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory(zydis EXCLUDE_FROM_ALL)

# Imgui
add_library(imgui STATIC)

target_sources(imgui PRIVATE
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
    imgui/include
)

target_link_libraries(imgui PRIVATE SDL3-shared ${CMAKE_DL_LIBS} Zydis winpthread discord-rpc)


