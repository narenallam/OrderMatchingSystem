cmake_minimum_required(VERSION 3.15)
project(OrderMatchingSystem VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find dependencies with proper paths for Conan 2
# First, include the generated conan cmake file if it exists
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

# Find system boost instead of using Conan's
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
message(STATUS "Found Boost version: ${Boost_VERSION}")

# Find other dependencies with explicit required components
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_WITH_COVERAGE "Build with code coverage" OFF)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include ${Boost_INCLUDE_DIRS})

# Set compilation flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(BUILD_WITH_COVERAGE)
        add_compile_options(--coverage)
    endif()
endif()

# Add subdirectories
add_subdirectory(utils)
add_subdirectory(orderMatcher)

# Create main executable
add_executable(run Main.cpp)

# Make sure the libraries are linked in the correct order
target_link_libraries(run 
    PRIVATE 
    order_matcher 
    logger 
    spdlog::spdlog
    fmt::fmt
    Boost::boost
    ${CMAKE_THREAD_LIBS_INIT}
)

# Build tests if enabled
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Install configuration
install(TARGETS run
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Package configuration
include(CPack)