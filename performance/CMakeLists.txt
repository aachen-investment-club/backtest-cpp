add_executable(analyze run_performance_analysis.cpp)

# Use the same warnings as the rest of the project
target_link_libraries(analyze PRIVATE BacktestCpp::Warnings)

# Set output directory to match user's request if needed, 
# but CMake usually handles this. 
# By default it will be in CMAKE_CURRENT_BINARY_DIR.
