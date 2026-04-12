cmake_minimum_required(VERSION 3.21)
project(BlockOverlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as shared library (.so)
add_library(blockoverlay SHARED
    src/main.cpp
)

# Compiler flags
target_compile_options(blockoverlay PRIVATE
    -fPIC
    -O2
    -ffunction-sections
    -fdata-sections
)

# Linker flags
target_link_options(blockoverlay PRIVATE
    -shared
    -Wl,--gc-sections
    -llog
)

target_compile_definitions(blockoverlay PRIVATE
    NDEBUG
    LEVILAUNCHROID_MOD
)
