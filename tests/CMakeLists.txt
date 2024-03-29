option(CO_ECS_CODE_COVERAGE "Enable coverage" OFF)

if(CO_ECS_CODE_COVERAGE)
  include(CodeCoverage)
endif(CO_ECS_CODE_COVERAGE)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

include(Catch)

add_executable(
  tests
  test_container.cpp
  test_component.cpp
  test_entity.cpp
  test_ecs.cpp
)

target_link_libraries(tests PRIVATE co_ecs PRIVATE Catch2::Catch2WithMain)

catch_discover_tests(tests)

add_subdirectory(test_shared)

if(CO_ECS_CODE_COVERAGE)
  append_coverage_compiler_flags()
  setup_target_for_coverage_gcovr_xml(
    NAME coverage
    EXECUTABLE tests
    BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/include"
  )
endif()
