Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.1
)

message(STATUS "TESTS: Fetching Catch2...")
FetchContent_MakeAvailable(Catch2)

add_executable(tests test_ecs.cpp)
target_include_directories(tests PRIVATE ..)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
enable_testing()
catch_discover_tests(tests TEST_SPEC "[ecs]")
