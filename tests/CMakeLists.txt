include(GoogleTest)

if (CMAKE_COMPILER_IS_GNUCC)
  option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang")

  if (ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
  endif()
endif()

if(MSVC)
# https://stackoverflow.com/questions/63925255/googletest-compile-with-mt-staticrelease-errors
set(gtest_force_shared_crt ON)
endif(MSVC)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0)
FetchContent_MakeAvailable(googletest)

add_executable(
  tests
  asl/test_hash_map.cpp
  asl/test_hash_set.cpp
  asl/test_sparse_set.cpp
  asl/test_sparse_map.cpp
  asl/test_handle.cpp
  asl/test_family.cpp
  asl/test_dynamic_bitset.cpp
  asl/test_string_utils.cpp
  ecs/test_component.cpp
  ecs/test_registry.cpp
  ecs/test_command.cpp
)

target_link_libraries(tests gtest gtest_main)

if (ENABLE_COVERAGE)
  target_link_options(tests PUBLIC --coverage)
endif()

gtest_discover_tests(tests)
