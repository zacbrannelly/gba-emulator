cmake_minimum_required(VERSION 3.10)

project(Emulator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build tests (ON by default)
option(BUILD_TESTS "Build the test runner" ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/3rdparty)
include_directories(${CMAKE_SOURCE_DIR})

# Define source files
set(COMMON_SOURCES
    cpu.cpp
    ram.cpp
    dma.cpp
    timer.cpp
    gpu.cpp
)

# Emulator executable
add_executable(emulator
    ${COMMON_SOURCES}
    emulator.cpp
)

# Optimization flag for release builds
target_compile_options(emulator PRIVATE $<$<CONFIG:RELEASE>:-O3>)

# Conditionally build tests
if(BUILD_TESTS)
    # Test runner executable
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

    add_executable(test_runner
        ${COMMON_SOURCES}
        ${TEST_SOURCES}
        3rdparty/catch_amalgamated.cpp
    )

    # Custom target for running tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_BINARY_DIR}/test_runner
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    add_custom_target(debug_tests
        COMMAND lldb ${CMAKE_BINARY_DIR}/test_runner
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
