include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.5.zip
    SYSTEM
    FIND_PACKAGE_ARGS NAMES benchmark
)

block(SCOPE_FOR VARIABLES)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    FetchContent_MakeAvailable(googlebenchmark)
endblock()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bm_data.cpp")
    add_executable(system_benchmarks bm_data.cpp)

    target_link_libraries(system_benchmarks 
        PRIVATE 
            BacktestCpp::Core 
            benchmark::benchmark 
            benchmark::benchmark_main
            BacktestCpp::Warnings
    )
    
    set_target_properties(system_benchmarks PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
endif()
