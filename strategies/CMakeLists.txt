file(GLOB BACKTESTCPP_STRATEGY_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
)

add_library(backtest_strategies STATIC)

set_target_properties(backtest_strategies PROPERTIES 
    EXPORT_NAME StrategiesArchive
    INTERPROCEDURAL_OPTIMIZATION ${BACKTESTCPP_ENABLE_LTO}
)

target_compile_features(backtest_strategies PUBLIC cxx_std_20)

target_link_libraries(backtest_strategies 
    PUBLIC 
        BacktestCpp::Core
    PRIVATE 
        $<BUILD_INTERFACE:BacktestCpp::Warnings>
)

target_sources(backtest_strategies
    PRIVATE
        ${BACKTESTCPP_STRATEGY_SOURCES}
        strategy_factory.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/include/backtest-cpp/strategies/strategy_factory.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/backtest-cpp/strategies/smacrossover.h"
)

add_library(backtest_strategies_whole INTERFACE)
add_library(BacktestCpp::Strategies ALIAS backtest_strategies_whole)

set_target_properties(backtest_strategies_whole PROPERTIES
    EXPORT_NAME Strategies
)

target_link_libraries(backtest_strategies_whole
    INTERFACE
        "$<BUILD_INTERFACE:$<LINK_LIBRARY:WHOLE_ARCHIVE,backtest_strategies>>"
        "$<INSTALL_INTERFACE:$<LINK_LIBRARY:WHOLE_ARCHIVE,BacktestCpp::StrategiesArchive>>"
)

install(TARGETS backtest_strategies backtest_strategies_whole
    EXPORT BacktestCppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)