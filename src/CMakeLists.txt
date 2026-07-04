add_library(backtest_core)
add_library(BacktestCpp::Core ALIAS backtest_core)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    csv_parser
    URL https://vincentlaucsb.github.io/csv-parser/csv.hpp
    DOWNLOAD_NO_EXTRACT TRUE
    SYSTEM
)
FetchContent_MakeAvailable(csv_parser)

set_target_properties(backtest_core PROPERTIES 
    EXPORT_NAME Core
    INTERPROCEDURAL_OPTIMIZATION ${BACKTESTCPP_ENABLE_LTO}
)

target_compile_features(backtest_core PUBLIC cxx_std_20)

find_package(Threads REQUIRED)

target_include_directories(backtest_core SYSTEM PRIVATE "${csv_parser_SOURCE_DIR}")
target_link_libraries(backtest_core 
    PRIVATE 
        $<BUILD_INTERFACE:BacktestCpp::Warnings>
        Threads::Threads
)

target_sources(backtest_core
    PRIVATE
        data.cpp
        performance.cpp
        portfolio.cpp
        symbol_dictionary.cpp
        utils.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
        FILES
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/data.h"
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/performance.h"
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/portfolio.h"
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/strategy.h"
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/symbol_dictionary.h"
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/types.h"
            "${PROJECT_SOURCE_DIR}/include/backtest-cpp/utils.h"
)

install(TARGETS backtest_core
    EXPORT BacktestCppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(backtest main.cpp)

    target_compile_definitions(backtest PRIVATE 
        DATA_INSTALL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}"
    )

    target_link_libraries(backtest 
        PRIVATE 
            BacktestCpp::Core 
            BacktestCpp::Strategies
            BacktestCpp::Warnings
    )

    set_target_properties(backtest PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION ${BACKTESTCPP_ENABLE_LTO}
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        XCODE_SCHEME_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )

    install(TARGETS backtest
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()
