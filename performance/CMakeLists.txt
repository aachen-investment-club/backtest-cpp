add_executable(analyze run_performance_analysis.cpp)

# Use the same warnings as the rest of the project
target_link_libraries(analyze PRIVATE BacktestCpp::Warnings)

target_include_directories(analyze PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)