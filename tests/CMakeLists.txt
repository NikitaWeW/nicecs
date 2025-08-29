Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.1
)

message(STATUS "TESTS: Fetching Catch2...")
FetchContent_MakeAvailable(Catch2)

file(GLOB_RECURSE TESTS_SOURCES "*.cpp")

add_executable(tests ${TESTS_SOURCES})
target_include_directories(tests PRIVATE ..)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
