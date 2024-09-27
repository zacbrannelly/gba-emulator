cmake_minimum_required(VERSION 3.10)

project(Emulator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Option to build tests (ON by default)
option(BUILD_TESTS "Build the test runner" ON)

# Option to build tests only (OFF by default)
option(CI_RUNNER "Build only the test runner" OFF)

# Define source files
set(COMMON_SOURCES
    src/cpu.cpp
    src/ram.cpp
    src/dma.cpp
    src/timer.cpp
    src/gpu.cpp
    src/state_io.cpp
)

# Only build emulator if not running in CI
if (NOT CI_RUNNER)
    # Include ZEngine (for rendering)
    add_subdirectory(3rdparty/zengine)

    find_package(glm REQUIRED)
    find_package(glfw3 3.3 REQUIRED)

    # Get include directories for dependencies
    get_target_property(GLFW_INCLUDE_DIRS glfw INTERFACE_INCLUDE_DIRECTORIES)
    set(BGFX_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zengine/3rdparty/bgfx.cmake/bgfx/include)
    set(IMGUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zengine/3rdparty/imgui.cmake/imgui)
    set(GUIZMO_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zengine/3rdparty/imgui.cmake/guizmo)
    set(ICON_FONT_HEADERS_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/zengine/3rdparty/iconfontheaders)

    # Emulator executable
    add_executable(emulator
        ${COMMON_SOURCES}
        src/debugger/palette_debugger.cpp
        src/debugger/sprite_debugger.cpp
        src/debugger/ram_debugger.cpp
        src/debugger/special_effects_debugger.cpp
        src/debugger/window_debugger.cpp
        src/debugger/bg_debugger.cpp
        src/debugger/cpu_debugger.cpp
        src/debugger/state_debugger.cpp
        src/input.cpp
        src/emulator.cpp
    )

    # Add include directories
    target_include_directories(emulator PRIVATE ${GLFW_INCLUDE_DIRS})
    target_include_directories(emulator PRIVATE ${BGFX_INCLUDE_DIRS})
    target_include_directories(emulator PRIVATE ${IMGUI_INCLUDE_DIRS})
    target_include_directories(emulator PRIVATE ${GUIZMO_INCLUDE_DIRS})
    target_include_directories(emulator PRIVATE ${ICON_FONT_HEADERS_INCLUDE_DIRS})
    target_include_directories(emulator PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty)
    target_include_directories(emulator PRIVATE ${CMAKE_SOURCE_DIR})

    # Link ZEngine
    target_link_libraries(emulator PRIVATE ZEngine-Core)
endif()

# Conditionally build tests
if(BUILD_TESTS)
    # Test runner executable
    file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

    add_executable(test_runner
        ${COMMON_SOURCES}
        ${TEST_SOURCES}
        3rdparty/catch_amalgamated.cpp
    )

    target_include_directories(test_runner PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_include_directories(test_runner PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty)

    # Custom target for running tests
    add_custom_target(run_tests
        COMMAND ${CMAKE_BINARY_DIR}/test_runner
        DEPENDS test_runner
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
