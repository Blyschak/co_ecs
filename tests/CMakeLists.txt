option(CO_ECS_CODE_COVERAGE "Enable coverage" OFF)

if(CO_ECS_CODE_COVERAGE)
  include(CodeCoverage)
endif(CO_ECS_CODE_COVERAGE)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG 7ce35799767de7b9c6ba836c72e479c5f70219a3
)
FetchContent_MakeAvailable(Catch2)

include(Catch)

add_executable(
  tests
  test_container.cpp
  test_component.cpp
  test_entity.cpp
  test_ecs.cpp
  test_schedule.cpp
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
