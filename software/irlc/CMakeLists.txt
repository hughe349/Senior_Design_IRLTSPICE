cmake_minimum_required(VERSION 3.30)

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

# CPM
# download CPM.cmake
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=f3a6dcc6a04ce9e7f51a127307fa4f699fb2bade357a8eb4c5b45df76e1dc6a5
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

# Boost
set(BOOST_INCLUDE_LIBRARIES graph program_options asio outcome)
CPMAddPackage(
    NAME Boost 
    VERSION 1.90.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
    URL_HASH SHA256=aca59f889f0f32028ad88ba6764582b63c916ce5f77b31289ad19421a96c555f
)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/numbers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/emit.cpp
)
target_include_directories(libirlc 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
target_link_libraries(libirlc 
    PRIVATE 
        Boost::graph
        Boost::asio
        Boost::outcome
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
