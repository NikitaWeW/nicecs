Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.9.1
)
FetchContent_Declare(
    EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.15.0
)

message(STATUS "BENCHMARKS: Fetching Catch2...")
FetchContent_MakeAvailable(Catch2)
message(STATUS "BENCHMARKS: Fetching EnTT...")
FetchContent_MakeAvailable(EnTT)

file(GLOB_RECURSE BENCHMARKS_SOURCES "*.cpp")

add_executable(benchmarks ${BENCHMARKS_SOURCES})
target_include_directories(benchmarks PRIVATE ..)
target_link_libraries(benchmarks PRIVATE Catch2::Catch2WithMain EnTT::EnTT)
