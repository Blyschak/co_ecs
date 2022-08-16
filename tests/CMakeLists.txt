
include(GoogleTest)

if(CODE_COVERAGE)
include(CodeCoverage)
endif(CODE_COVERAGE)

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

gtest_discover_tests(tests)

if(CODE_COVERAGE)
append_coverage_compiler_flags()
setup_target_for_coverage_gcovr_html(
  NAME coverage
  EXECUTABLE tests
  BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/include"
)
endif()