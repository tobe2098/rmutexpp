# rmutexpp/examples/CMakeLists.txt
add_executable(rmutex_example main.cpp)

# Link against your library target
target_link_libraries(rmutex_example PRIVATE rmutexpp_core)

# If your example needs specific C++ standard features beyond what the library provides
# target_compile_features(rmutex_example PRIVATE cxx_std_17)