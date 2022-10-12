add_executable(test_shared_lib test_shared_lib.cpp)

add_library(test_lib SHARED test_lib.cpp)

target_link_libraries(test_lib PRIVATE co_ecs)

target_link_libraries(
    test_shared_lib
    PRIVATE co_ecs
    PRIVATE test_lib
    PRIVATE Catch2::Catch2WithMain
)

catch_discover_tests(test_shared_lib)