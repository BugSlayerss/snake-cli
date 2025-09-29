cmake_minimum_required(VERSION 3.14)
project(snake_tests)

set(CMAKE_CXX_STANDARD 17)

# Download and configure GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_SOURCES
  snake_tests.cpp
)

# Build test executable
add_executable(snake_tests ${TEST_SOURCES})

# Add project headers (so snake.h is visible)
target_include_directories(snake_tests PRIVATE ${CMAKE_SOURCE_DIR}/../)

# Link with GoogleTest
target_link_libraries(snake_tests PRIVATE gtest gtest_main)

# pthread only needed on Linux/macOS
if (UNIX AND NOT WIN32)
    target_link_libraries(snake_tests PRIVATE pthread)
endif()

# Register tests
include(GoogleTest)
gtest_discover_tests(snake_tests)
