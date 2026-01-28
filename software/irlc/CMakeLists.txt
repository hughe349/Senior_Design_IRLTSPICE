cmake_minimum_required(VERSION 4.0)

project(IrlCompiler)

add_executable(irlc)
target_sources(irlc
  PRIVATE
    ./src/main.cpp
)
