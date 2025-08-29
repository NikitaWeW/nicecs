cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ecs CXX)

cmake_policy(SET CMP0076 NEW)

add_library(ecs INTERFACE)
target_sources(ecs PUBLIC ecs.hpp)
target_include_directories(ecs INTERFACE .)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
