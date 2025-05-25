cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(rmutexpp
    VERSION 1.0.0
    DESCRIPTION "Rust-inspired mutex and mutex guard for C++"
    LANGUAGES CXX
)

# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the header-only library target
add_library(rmutexpp_core INTERFACE)

# Create an alias target for consistent naming
add_library(rmutexpp::rmutexpp_core ALIAS rmutexpp_core)

# Specify include directories for consumers
target_include_directories(rmutexpp_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Specify required C++ features
target_compile_features(rmutexpp_core INTERFACE cxx_std_20)

target_compile_definitions(rmutexpp_core INTERFACE
    $<IF:$<CONFIG:Debug>,DEBUG_RMUTEX,>
)

# Only set up installation if this is the main project or if explicitly requested
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR RMUTEXPP_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Install the target
    install(TARGETS rmutexpp_core
        EXPORT rmutexppTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install headers
    install(DIRECTORY include/ 
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install the export set
    install(EXPORT rmutexppTargets
        FILE rmutexppTargets.cmake
        NAMESPACE rmutexpp::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rmutexpp
    )

    # Create and install config file
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/rmutexppConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/rmutexppConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rmutexpp
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/rmutexppConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/rmutexppConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/rmutexppConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rmutexpp
    )
endif()

# Only build tests and examples if this is the main project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    include(FetchContent)

    # Fetch Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(examples)
    add_subdirectory(test)
endif()