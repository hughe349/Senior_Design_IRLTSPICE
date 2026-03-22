cmake_minimum_required(VERSION 4.0)

# ========
# Includes
# ========
include(FetchContent)

project(IrlCompiler LANGUAGES CXX)

# =========
# Variables
# =========

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============
# Dependencies
# ============

# Boost
set(BOOST_INCLUDE_LIBRARIES graph program_options) 
FetchContent_Declare(
    Boost 
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.90.0
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(Boost)

# =======
# Targets
# =======

# Lib (separated for testing and such)
add_library(libirlc) 
target_sources(libirlc 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/compiler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/route.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/parse_spice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/parse_eechema.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/verify.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/debug.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/board_info.cpp
)
target_include_directories(libirlc 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
target_link_libraries(libirlc 
    PRIVATE 
        Boost::graph
)

# Actual compiler
add_executable(irlc)
target_sources(irlc 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)
target_link_libraries(irlc 
    PRIVATE 
        libirlc 
        Boost::program_options
)

# =======
# Testing
# =======
enable_testing()
add_subdirectory(tests)
