include(GoogleTest)

if(CODE_COVERAGE)
  include(CodeCoverage)
endif(CODE_COVERAGE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(
  tests
  co_ecs/detail/test_hash_map.cpp
  co_ecs/detail/test_sparse_map.cpp
  co_ecs/detail/test_dynamic_bitset.cpp
  co_ecs/test_entity.cpp
  co_ecs/test_component.cpp
  co_ecs/test_registry.cpp
)

target_link_libraries(tests co_ecs gtest gtest_main)

gtest_discover_tests(tests)

if(CODE_COVERAGE)
  append_coverage_compiler_flags()
  setup_target_for_coverage_gcovr_xml(
    NAME coverage
    EXECUTABLE tests
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/include"
  )
endif()
