cmake_minimum_required(VERSION 3.26...3.31)

project(BacktestCpp
    VERSION 0.1.0
    DESCRIPTION "A high-performance C++ Backtesting Engine"
    HOMEPAGE_URL "https://github.com/aachen-investment-club/backtest-cpp"
    LANGUAGES CXX
)

include(GNUInstallDirs)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

option(BACKTESTCPP_ENABLE_WERROR "Treat warnings as errors" OFF)
option(BACKTESTCPP_ENABLE_LTO "Enable Interprocedural Optimization (LTO)" ${PROJECT_IS_TOP_LEVEL})
option(BACKTESTCPP_BUILD_BENCHMARKS "Enable and build benchmarks" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_COMPILE_WARNING_AS_ERROR ${BACKTESTCPP_ENABLE_WERROR})

if(BACKTESTCPP_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(NOT ipo_supported)
        message(WARNING "IPO/LTO is not supported: ${ipo_error}")
        set(BACKTESTCPP_ENABLE_LTO OFF)
    endif()
endif()

add_library(backtest_warnings INTERFACE)
add_library(BacktestCpp::Warnings ALIAS backtest_warnings)

target_compile_options(backtest_warnings INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang,GNU>:
        -Wall;-Wextra;-Wshadow;-Wnon-virtual-dtor;-Wold-style-cast;-Wcast-align;-Wunused;-Woverloaded-virtual;-Wpedantic;-Wconversion;-Wsign-conversion>
    $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:
        /W4;/w14242;/w14254;/w14263;/w14265;/w14287;/w14296;/w14311;/w14545;/w14546;/w14547;/w14549;/w14555;/w14619;/w14640;/w14826;/w14905;/w14906;/w14928>
)

add_subdirectory(src)
add_subdirectory(strategies)
add_subdirectory(performance)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")

include(CTest)
if(BUILD_TESTING AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(tests)
endif()

if(BACKTESTCPP_BUILD_BENCHMARKS AND PROJECT_IS_TOP_LEVEL)
    add_subdirectory(benchmarks)
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(CMakePackageConfigHelpers)
    
    write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/BacktestCppConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/BacktestCppConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/BacktestCppConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BacktestCpp"
    )

    install(EXPORT BacktestCppTargets
        FILE BacktestCppTargets.cmake
        NAMESPACE BacktestCpp::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BacktestCpp"
    )

    install(FILES
        "${PROJECT_BINARY_DIR}/BacktestCppConfig.cmake"
        "${PROJECT_BINARY_DIR}/BacktestCppConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BacktestCpp"
    )

    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_PACKAGE_VENDOR "Aachen Investment Club")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
    set(CPACK_GENERATOR "TGZ;ZIP")
    include(CPack)
endif()
