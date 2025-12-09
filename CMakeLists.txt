cmake_minimum_required(VERSION 3.15)
project(BacktestEngine)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# ============================================================================
# Google Test Setup
# ============================================================================

# Fetch Google Test from GitHub
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
)

# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Make Google Test available
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# ============================================================================
# Main Executable
# ============================================================================

add_executable(backtest
    src/main.cpp
    src/data.cpp
    src/portfolio.cpp
    src/event.cpp
    ./strategies/SMACrossover.cpp
)

# ============================================================================
# Test Executable
# ============================================================================

add_executable(data_tests
    tests/test_data.cpp
    src/data.cpp
)

target_link_libraries(data_tests
    GTest::gtest_main
)

add_executable(portfolio_tests
    tests/test_portfolio.cpp
    src/data.cpp
    src/portfolio.cpp
)

target_link_libraries(portfolio_tests
    GTest::gtest_main
)

# Discover and register tests with CTest
include(GoogleTest)
gtest_discover_tests(data_tests)
gtest_discover_tests(portfolio_tests)

# ============================================================================
# Optional: Generate compile_commands.json for IDE integration
# ============================================================================

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)