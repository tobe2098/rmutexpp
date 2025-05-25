# rmutex_lib/test/CMakeLists.txt

# Create a test executable
add_executable(rmutex_unit_tests rmutex_unit_tests.cpp)

# Link your test executable to your library and GTest
# GTest::gtest_main provides a main() function for running tests automatically
target_link_libraries(rmutex_unit_tests PRIVATE
    rmutexpp_core       # Your internal library target
    GTest::gtest_main   # Google Test main library
)

# Ensure the test executable also uses C++20
target_compile_features(rmutex_unit_tests PRIVATE cxx_std_20)

# Register the test executable with CTest
add_test(NAME RMutexUnitTests COMMAND rmutex_unit_tests)