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
set(BOOST_INCLUDE_LIBRARIES graph)
FetchContent_Declare(
  Boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.90.0
  GIT_SHALLOW 1
)
FetchContent_MakeAvailable(Boost)

# =======
# Testing
# =======

enable_testing()
add_subdirectory(tests)

# =======
# Targets
# =======

add_executable(irlc)
target_sources(irlc
  PRIVATE
    ./src/main.cpp
)


