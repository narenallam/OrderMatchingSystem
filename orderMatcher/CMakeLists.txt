set(LIB_NAME order_matcher)

# Library sources
set(SOURCES
    OrderMatcher.cpp
)

# Find thread package for threading support
find_package(Threads REQUIRED)

# Create library target
add_library(${LIB_NAME} STATIC ${SOURCES})

# Set include directories
target_include_directories(${LIB_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/Include
)

# Link dependencies
target_link_libraries(${LIB_NAME} 
    PUBLIC logger
    PRIVATE Threads::Threads
    PRIVATE fmt::fmt
)

# Set properties
set_target_properties(${LIB_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)