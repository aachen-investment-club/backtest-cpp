add_library(backtest_strategies)
add_library(BacktestCpp::Strategies ALIAS backtest_strategies)

set_target_properties(backtest_strategies PROPERTIES 
    EXPORT_NAME Strategies
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
        SMACrossover.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/include/backtest-cpp/strategies/smacrossover.h"
)

install(TARGETS backtest_strategies
    EXPORT BacktestCppTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
