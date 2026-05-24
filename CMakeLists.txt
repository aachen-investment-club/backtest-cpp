cmake_minimum_required(VERSION 3.15)
project(BacktestEngine)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Build Type Configuration
# ============================================================================
# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING 
        "Choose: Debug or Release" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Configure compiler flags for GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Debug: For GDB debugging
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    
    # Release: For fast execution with hardware-specific optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
    
    # Optional: Release with debug info (fast but still debuggable)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -march=native")
endif()

# Enable LTO (Link Time Optimization) / IPO if supported
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    message(STATUS "LTO/IPO is supported and enabled.")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(WARNING "LTO/IPO is not supported: ${error}")
endif()

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
# Google Benchmark Setup
# ============================================================================
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# ============================================================================
# Core Library Setup
# ============================================================================
# Compile all core business logic into a single static library.
add_library(BacktestCore STATIC
    src/data.cpp
    src/utils.cpp
    src/portfolio.cpp
    strategies/SMACrossover.cpp
    src/performance.cpp
)

# Expose the 'include' directory to any target that links against BacktestCore
target_include_directories(BacktestCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# Main Executable
# ============================================================================
add_executable(backtest src/main.cpp)
target_link_libraries(backtest PRIVATE BacktestCore)

# ============================================================================
# Test Executables
# ============================================================================

add_executable(data_tests tests/test_data.cpp)
target_link_libraries(data_tests PRIVATE BacktestCore GTest::gtest_main)

add_executable(portfolio_tests tests/test_portfolio.cpp)
target_link_libraries(portfolio_tests PRIVATE BacktestCore GTest::gtest_main)

add_executable(performance_tests tests/test_performance.cpp)
target_link_libraries(performance_tests PRIVATE BacktestCore GTest::gtest_main)

# Discover and register tests with CTest
include(GoogleTest)
gtest_discover_tests(data_tests)
gtest_discover_tests(portfolio_tests)
gtest_discover_tests(performance_tests)

# ============================================================================
# Microbenchmark Executable
# ============================================================================
add_executable(system_benchmarks 
    benchmarks/bm_data.cpp
)

target_link_libraries(system_benchmarks 
    PRIVATE 
    BacktestCore 
    benchmark::benchmark 
    benchmark::benchmark_main
)

# ============================================================================
# Optional: Generate compile_commands.json for IDE integration
# ============================================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)