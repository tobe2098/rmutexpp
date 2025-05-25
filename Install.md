## `rmutexpp` Installation Guide (with CMake)

This guide assumes you have **CMake (version 3.14 or higher)** and a C++20 compatible compiler (GCC, Clang, MSVC) installed on your system.

### Option 1: Manual Installation from Source

This method involves downloading `rmutexpp`'s source code, building its CMake configuration, and "installing" the necessary files (headers and CMake configuration) to a specified location.

1.  **Get the Source Code:**
    ```bash
    git clone https://github.com/YOUR_USERNAME/rmutex_lib.git
    cd rmutex_lib
    ```
    (Replace `YOUR_USERNAME` with your actual GitHub username)

2.  **Build and Install:**
    Create a build directory, configure CMake, build, and then install.

    ```bash
    mkdir build
    cd build
    # Configure for Release mode (recommended for installation)
    cmake .. -DCMAKE_BUILD_TYPE=Release
    # Build the project (including generating CMake config files)
    cmake --build . --config Release # Use --config Release on Windows
    # Install the library to a system-wide or custom prefix
    # Common prefixes: /usr/local (Linux/macOS), C:/Program Files (Windows)
    # Or a custom path, e.g., $HOME/my_libs/rmutexpp
    cmake --install . --prefix /usr/local
    ```
    * Replace `/usr/local` with your desired installation path if you don't want a system-wide install.

3.  **Usage in Your Project (after manual install):**

    In your `CMakeLists.txt`, you can now find and link to `rmutexpp`.

    ```cmake
    # my_consuming_project/CMakeLists.txt
    cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
    project(MyRMutexApp LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Find the installed rmutexpp library.
    # CMake will search in standard installation paths like /usr/local/lib/cmake/rmutexpp
    # If installed to a custom path (e.g., $HOME/my_libs/rmutexpp),
    # you might need to set CMAKE_PREFIX_PATH before the cmake command:
    # export CMAKE_PREFIX_PATH=$HOME/my_libs/rmutexpp:$CMAKE_PREFIX_PATH
    find_package(rmutexpp CONFIG REQUIRED)

    # Create your executable
    add_executable(my_app main.cpp)

    # Link against the rmutexpp::Core target provided by the library.
    # PRIVATE linkage is typically sufficient for header-only libraries used by an executable.
    target_link_libraries(my_app PRIVATE rmutexpp::Core)
    ```

### Option 2: Using `WorkspaceContent` (Recommended for Modern CMake)

`WorkspaceContent` allows you to integrate `rmutexpp` directly into your project's build process by fetching its source code at CMake configure time. This means you don't need to manually install `rmutexpp` beforehand. It's often preferred for header-only libraries as it ensures you get the exact version needed without system-wide dependencies.

1.  **Modify Your Project's `CMakeLists.txt`:**

    Add the following lines to your consuming project's `CMakeLists.txt` (before `add_executable` or `add_library` where you use `rmutexpp`).

    ```cmake
    # my_consuming_project/CMakeLists.txt
    cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
    project(MyRMutexApp LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # --- FetchContent Integration ---
    # Include the FetchContent module
    include(FetchContent)

    # Declare the rmutexpp library as a content to fetch
    FetchContent_Declare(
        rmutexpp_lib_content # Internal name for FetchContent
        GIT_REPOSITORY https://github.com/YOUR_USERNAME/rmutex_lib.git
        GIT_TAG        main # Or a specific Git tag, e.g., 'v1.0.0', or a commit hash
    )

    # Make the content available. This downloads the source and adds its CMakeLists.txt
    # via add_subdirectory, making its targets available in your build.
    FetchContent_MakeAvailable(rmutexpp_lib_content)

    # Important: Although FetchContent_MakeAvailable adds the project,
    # calling find_package is still the idiomatic and most robust way to ensure
    # that the imported targets (like rmutexpp::Core) are properly configured
    # in your project, especially if the external project uses complex export logic.
    find_package(rmutexpp CONFIG REQUIRED)
    # --- End FetchContent Integration ---


    # Create your executable
    add_executable(my_app main.cpp)

    # Link against the rmutexpp::Core target.
    # This target is now available because FetchContent made it part of your build,
    # and find_package confirmed its properties.
    target_link_libraries(my_app PRIVATE rmutexpp::Core)
    ```

2.  **Include Headers in Your Source Code:**

    In your C++ source files (`main.cpp`, etc.), you can now include `rmutexpp` headers as usual:

    ```cpp
    // my_consuming_project/main.cpp
    #include <iostream>
    #include <string>
    #include "rmutexpp/rmutex.hpp" // Use the installed/fetched include path
    #include "rmutexpp/rmutex_guard.hpp"

    int main() {
        rmutexpp::rmutex<std::string> my_data{"Hello, RMutex!"};

        {
            auto guard = my_data.lock();
            std::cout << *guard << std::endl;
            *guard = "Data modified!";
        } // Lock is released here

        std::cout << "After guard: " << *my_data.lock() << std::endl;

        return 0;
    }
    ```

### Important Notes:

* **C++ Standard Consistency:** Ensure your consuming project uses the same C++ standard (C++20) or a compatible one as `rmutexpp` (which uses C++20 features like concepts, structured bindings).
* **Header-Only Library:** `rmutexpp` is a header-only library. This means there's no actual `.lib` or `.so` file generated for `rmutexpp_Core`. CMake primarily generates the necessary include paths and compiler definitions for consuming projects.
* **`PRIVATE` vs. `PUBLIC` vs. `INTERFACE` Linkage:**
    * When you link `rmutexpp::Core` with `PRIVATE` in `target_link_libraries`, it means only `my_app` needs `rmutexpp::Core`'s headers and definitions.
    * If `my_app` was itself a library and its public headers exposed types from `rmutexpp`, you might use `PUBLIC` or `INTERFACE` linkage. For a header-only library, `INTERFACE` on your `rmutexpp_Core` target ensures its public headers and definitions are propagated.