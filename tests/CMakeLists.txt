include(GoogleTest)

option(CO_ECS_CODE_COVERAGE "Enable coverage" OFF)

if(CO_ECS_CODE_COVERAGE)
  include(CodeCoverage)
endif(CO_ECS_CODE_COVERAGE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

include(Catch)

add_executable(
  tests
  co_ecs/detail/test_hash_map.cpp
  co_ecs/detail/test_sparse_map.cpp
  co_ecs/detail/test_dynamic_bitset.cpp
  co_ecs/test_entity.cpp
  co_ecs/test_component.cpp
)

add_executable(
  tests2
  test_container.cpp
  test_component.cpp
  test_entity.cpp
  test_ecs.cpp
)

target_link_libraries(tests co_ecs gtest gtest_main)

target_link_libraries(tests2 PRIVATE co_ecs PRIVATE Catch2::Catch2WithMain)

gtest_discover_tests(tests)

catch_discover_tests(tests2)

if(CO_ECS_CODE_COVERAGE)
  append_coverage_compiler_flags()
  setup_target_for_coverage_gcovr_xml(
    NAME coverage
    EXECUTABLE tests
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/include"
  )
endif()
