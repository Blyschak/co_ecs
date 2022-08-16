set(BENCHMARK_ENABLE_TESTING OFF)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark
  GIT_TAG 3b3de69400164013199ea448f051d94d7fc7d81f)
FetchContent_MakeAvailable(benchmark)

add_executable(benchmarks ecs/registry.cpp main.cpp)

target_link_libraries(benchmarks benchmark::benchmark)
