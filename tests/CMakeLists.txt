include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
    SYSTEM
    FIND_PACKAGE_ARGS NAMES GTest
)
FetchContent_Declare(
    csv_parser
    URL https://vincentlaucsb.github.io/csv-parser/csv.hpp
    DOWNLOAD_NO_EXTRACT TRUE
    SYSTEM
)
FetchContent_MakeAvailable(csv_parser)

block(SCOPE_FOR VARIABLES)
    set(gtest_force_shared_crt ON)
    set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)
endblock()

include(GoogleTest)

set(TEST_UNITS data portfolio performance strategy)

foreach(UNIT IN LISTS TEST_UNITS)
    set(TEST_TARGET "test_${UNIT}")
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_TARGET}.cpp")
        add_executable(${TEST_TARGET} ${TEST_TARGET}.cpp)
        
        target_compile_definitions(${TEST_TARGET} PRIVATE 
            PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}"
        )
        
        target_link_libraries(${TEST_TARGET} 
            PRIVATE 
                BacktestCpp::Core 
                BacktestCpp::Strategies
                GTest::gtest_main
                BacktestCpp::Warnings
        )
        
        set_target_properties(${TEST_TARGET} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            XCODE_SCHEME_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        )

        gtest_discover_tests(${TEST_TARGET})
    endif()
endforeach()
