set(LIB_NAME order_matcher)

# Library sources
set(SOURCES
    OrderMatcher.cpp
    HighPerformanceOrderBook.cpp
)

# Find dependencies
find_package(Threads REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

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
    PUBLIC fmt::fmt
    PUBLIC spdlog::spdlog
    PRIVATE Threads::Threads
)

# Set properties
set_target_properties(${LIB_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)