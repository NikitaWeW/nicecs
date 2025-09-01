cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ECS_BUILD_BENCHMARKS OFF "build esc benchmarks")
option(ECS_BUILD_TESTS OFF "build esc tests")

project(ECS CXX)

cmake_policy(SET CMP0076 NEW)

add_library(ecs INTERFACE)
target_sources(ecs PUBLIC ecs.hpp)
target_include_directories(ecs INTERFACE .)

if(ECS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
