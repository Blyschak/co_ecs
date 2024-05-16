function(add_failing_test target_name)
    # Create the executable target from the specified source file
    add_executable(${target_name} ${target_name}.cpp)
    # Link the necessary libraries (modify as needed)
    target_link_libraries(${target_name} PRIVATE co_ecs)
    # Set properties to exclude this target from all builds by default
    set_target_properties(${target_name} PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
    )

    # Add a test that expects this build target to fail
    add_test(
        NAME ${target_name}
        COMMAND ${CMAKE_COMMAND} --build . --target ${target_name} --config $<CONFIG>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # Specify that the test should expect a failure
    set_tests_properties(${target_name} PROPERTIES WILL_FAIL true)
endfunction()

add_failing_test(test_entity_create_with_repeating_components)