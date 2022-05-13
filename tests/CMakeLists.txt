include(GoogleTest)

# https://stackoverflow.com/questions/63925255/googletest-compile-with-mt-staticrelease-errors
option(gtest_force_shared_crt ON)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.11.0)
FetchContent_MakeAvailable(googletest)

add_executable(
  tests
  asl/test_algorithm.cpp
  asl/test_hash_map.cpp
  asl/test_hash_set.cpp
  asl/test_sparse_set.cpp
  asl/test_sparse_map.cpp
  asl/test_handle.cpp
  asl/test_family.cpp
  asl/test_defer.cpp
  asl/test_zip.cpp
  asl/test_dynamic_bitset.cpp
  ecs/test_component.cpp
  ecs/test_registry.cpp
  ecs/test_system.cpp
  ecs/test_scheduler.cpp
  scripting/test_lua_engine.cpp
  core/test_config.cpp)

target_link_libraries(tests engine gtest gtest_main)

gtest_discover_tests(tests)
