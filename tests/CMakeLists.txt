set(TEST_NAME runtests)

# Test sources
set(TEST_SOURCES
    AllTests.cpp
)

# Add the test executable
add_executable(${TEST_NAME} ${TEST_SOURCES})

# Set include directories
target_include_directories(${TEST_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/Include
)

# Link dependencies
target_link_libraries(${TEST_NAME}
    PRIVATE order_matcher
    PRIVATE logger
    PRIVATE Boost::unit_test_framework
    PRIVATE fmt::fmt
)

# Add test to CTest
add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
enable_testing()

# Add stress test for concurrent operations
set(STRESS_TEST_NAME concurrent_stress_test)
add_executable(${STRESS_TEST_NAME} ConcurrentStressTest.cpp)
target_link_libraries(${STRESS_TEST_NAME}
    PRIVATE order_matcher
    PRIVATE logger
    PRIVATE Boost::unit_test_framework
    PRIVATE fmt::fmt
)
target_include_directories(${STRESS_TEST_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/Include
)

# Add property-based test for order matching logic
set(PROP_TEST_NAME property_based_test)
add_executable(${PROP_TEST_NAME} PropertyBasedTest.cpp)
target_link_libraries(${PROP_TEST_NAME}
    PRIVATE order_matcher
    PRIVATE logger
    PRIVATE Boost::unit_test_framework
    PRIVATE fmt::fmt
)
target_include_directories(${PROP_TEST_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/Include
)

# Add code coverage target if enabled
if(BUILD_WITH_COVERAGE)
    add_custom_target(coverage
        COMMAND lcov --directory . --capture --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '*/ExternalLibraries/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${TEST_NAME}
        COMMENT "Generating code coverage report"
    )
endif()