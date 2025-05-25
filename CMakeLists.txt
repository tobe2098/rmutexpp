cmake_minimum_required(VERSION 3.14 FATAL_ERROR) # 3.14 or higher is recommended for FetchContent

project(rmutexpp
    VERSION 1.0.0
    DESCRIPTION "Rust-inspired mutex and mutex guard for C++"
    LANGUAGES CXX
)

# Enforce C++17 standard, as your code uses features like structured bindings, optional, etc.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard C++

# Define the header-only library target
# INTERFACE is used for header-only libraries as they don't produce object files themselves
add_library(rmutexpp_core INTERFACE)

# Specify include directories for consumers
target_include_directories(rmutexpp_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # When installed, headers will be in 'include'
)

# Specify required C++ features (e.g., for optional, structured bindings)
target_compile_features(rmutexpp_core INTERFACE cxx_std_20)

# Install rules for when someone builds/installs your library directly,
# or when FetchContent "installs" it into its build directory.
install(TARGETS rmutexpp_core EXPORT rmutexppTargets)
install(DIRECTORY include/ DESTINATION include)

# Export targets so they can be found by find_package (and thus FetchContent)
install(EXPORT rmutexppTargets
  FILE rmutexppTargets.cmake
  NAMESPACE rmutexpp:: # Adds a namespace to the imported targets, e.g., rmutexpp_core
  DESTINATION lib/cmake/rmutexpp
)

# You can also add a basic config file for find_package
configure_file(rmutexppConfig.cmake.in rmutexppConfig.cmake @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rmutexppConfig.cmake"
        DESTINATION lib/cmake/rmutexpp)


# Optional: Add an example project
add_subdirectory(examples)

# Optional: Add testing if you use CTest/GTest etc.
# enable_testing()
# add_subdirectory(test)