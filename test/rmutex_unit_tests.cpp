// rmutex_lib/test/rmutex_unit_tests.cpp

// Standard library headers for various functionalities
#include <chrono>    // For std::chrono::milliseconds and std::this_thread::sleep_for
#include <optional>  // For std::optional, used by try_lock results
#include <string>    // For std::string
#include <thread>    // For std::thread, used in concurrency tests
#include <vector>    // For std::vector (though not directly used in the current tests, often useful)

// Google Test framework header
#include "gtest/gtest.h"

// Headers for the rmutexpp library components being tested
#include "rmutexpp/rmutex.hpp"
#include "rmutexpp/rmutex_guard.hpp"

// Use the rmutexpp namespace for convenience within this test file.
// This allows direct use of rmutex, rmutex_ref, etc., without prefixing.
using namespace rmutexpp;

// --- Test Fixture Definition ---
// A test fixture provides a common setup (and teardown) for a group of related tests.
// It ensures each test starts with a fresh, known state, promoting test isolation.
struct rmutexTest : public ::testing::Test {
    // Member mutexes are automatically re-initialized for each test method (TEST_F).
    rmutex<std::string> test_mutex { "initial" };
    rmutex<int>         int_mutex { 0 };

    // You could also add SetUp() and TearDown() methods here for more complex setup/teardown
    // void SetUp() override { /* Code to run before each test */ }
    // void TearDown() override { /* Code to run after each test */ }
};

// --- Test Cases ---

// TEST_F indicates a test case that uses the 'rmutexTest' fixture.
// This test verifies the basic locking and data modification functionality of rmutex_ref.
TEST_F(rmutexTest, rmutex_refBasicLock) {
  // Scope for the first rmutex_ref, ensuring its lock is released automatically.
  {
    // Acquire a lock on test_mutex and get a reference to its guarded data.
    rmutex_ref<std::string> ref = test_mutex.lock();
    // Assert that the initial value is as expected.
    ASSERT_EQ(*ref, "initial");
    // Modify the guarded data through the reference.
    *ref = "modified";
    // Assert that the modification took effect.
    ASSERT_EQ(*ref, "modified");
  }  // 'ref' goes out of scope here, releasing the lock on 'test_mutex'.

  // After the lock is released, verify that the changes persisted and
  // the mutex can be locked again to access the modified data.
  rmutex_ref<std::string> ref2 = test_mutex.lock();
  ASSERT_EQ(*ref2, "modified");
}

// This test verifies the non-blocking 'try_lock' functionality of rmutex.
TEST_F(rmutexTest, rmutex_refTryLock) {
  // Acquire an initial lock to make subsequent try_lock attempts fail.
  rmutex_ref<std::string> ref = test_mutex.lock();
  // Confirm the initial lock was successfully acquired.
  ASSERT_TRUE(ref);
  ASSERT_EQ(*ref, "initial");  // Ensure data is still initial before attempting try_lock

  // Attempt to acquire the lock again (simulating another thread's attempt).
  // This should fail because 'ref' still holds the lock.
  std::optional<rmutex_ref<std::string>> optional_ref = test_mutex.try_lock();
  ASSERT_FALSE(optional_ref);  // Assert that the try_lock attempt failed.

  // Manually call the destructor of 'ref' to release the lock.
  // NOTE: In real application code, you would simply let 'ref' go out of scope.
  // This explicit call is primarily for specific test control.
  ref.~rmutex_ref();

  // Now that the lock is released, try_lock should succeed.
  optional_ref = test_mutex.try_lock();
  ASSERT_TRUE(optional_ref);  // Assert that the try_lock attempt succeeded.
  // Data should still be its initial state (no modification occurred in this test so far).
  ASSERT_EQ(*(*optional_ref), "initial");
}

// This test verifies rmutex_guard's ability to acquire and manage locks on multiple mutexes.
TEST_F(rmutexTest, rmutex_guardMultiLock) {
  // Create two separate rmutex instances with different data types.
  rmutex<std::string> mutex1 { "data1" };
  rmutex<int>         mutex2 { 100 };

  // Scope for rmutex_guard, ensuring locks are released upon exit.
  {
    // Create an rmutex_guard to acquire locks on both mutex1 and mutex2.
    // rmutex_guard handles the safe, ordered acquisition of multiple locks.
    rmutex_guard guard { mutex1, mutex2 };
    // Assert that the guard successfully acquired all its managed locks.
    ASSERT_TRUE(guard.owns());

    // Use structured binding to get references to the guarded data from the tuple.
    // 'auto&' is crucial here to get references, allowing modification of original data.
    auto [s_ref, i_ref] = *guard.get_data();
    // Modify the data through the acquired references.
    s_ref = "new_data";
    i_ref += 1;  // Increment the integer data

    // Assert that the modifications are reflected in the references.
    ASSERT_EQ(s_ref, "new_data");
    ASSERT_EQ(i_ref, 101);
  }  // 'guard' goes out of scope here, releasing locks on both mutex1 and mutex2.

  // After locks are released, acquire them individually to verify the changes persisted.
  ASSERT_EQ(*(mutex1.lock()), "new_data");
  ASSERT_EQ(*(mutex2.lock()), 101);
}

// This test focuses on verifying rmutex_guard's deadlock prevention mechanism
// when multiple threads try to acquire locks in different orders.
TEST_F(rmutexTest, rmutex_guardDeadlockPrevention) {
  // Create two rmutex instances.
  rmutex<int> m1 { 1 };
  rmutex<int> m2 { 2 };

  // Flags to track if threads have completed their work.
  bool thread1_done = false;
  bool thread2_done = false;

  // Thread 1: Attempts to lock m1 then m2 using rmutex_guard.
  // rmutex_guard internally uses std::lock, which prevents deadlocks by acquiring locks in a consistent order.
  std::thread t1([&]() {
    rmutex_guard guard { m1, m2 };          // Locks m1 then m2 (or consistent order)
    auto [num1, num2] = *guard.get_data();  // Get references to actual data
    num1              = 10;
    num2              = 20;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Simulate work under lock
    thread1_done = true;
  });

  // Thread 2: Attempts to lock m2 then m1 using rmutex_guard.
  // This order (if not handled by std::lock) would typically cause a deadlock with Thread 1.
  std::thread t2([&]() {
    rmutex_guard guard { m2, m1 };                               // Locks m2 then m1 (or consistent order)
    auto [num2, num1] = *guard.get_data();                       // Get references to actual data (order matters in structured binding)
    num1              = 100;                                     // This updates m1
    num2              = 200;                                     // This updates m2
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Simulate work under lock
    thread2_done = true;
  });

  // Wait for both threads to complete.
  t1.join();
  t2.join();

  // Assert that both threads successfully finished their execution.
  ASSERT_TRUE(thread1_done);
  ASSERT_TRUE(thread2_done);

  // Verify the final state of the mutexes. Due to the nature of thread scheduling,
  // either thread's writes could be the final state. We assert that one of the two
  // expected outcomes (from t1 or t2) occurred.
  int final_m1_val = *m1.lock();  // Acquire lock to read final value safely
  int final_m2_val = *m2.lock();  // Acquire lock to read final value safely

  std::cout << "Final m1: " << final_m1_val << ", Final m2: " << final_m2_val << std::endl;

  ASSERT_TRUE((final_m1_val == 10 && final_m2_val == 20) || (final_m1_val == 100 && final_m2_val == 200));
}

// Additional test cases can be added below this line.
// Examples include:
// - Testing const access to guarded data.
// - Verifying behavior with different data types (e.g., custom objects).
// - Testing move semantics for RMutex or RMutexRef/RMutexGuard.
// - Edge cases like empty mutexes (if applicable).
// - Performance benchmarks (though typically in separate benchmark suites).

// The main function for running Google Tests is usually provided by linking to GTest::gtest_main.
// If you manually wanted to specify it, it would look like this:
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv); // Initializes Google Test framework
//     return RUN_ALL_TESTS();                 // Runs all registered tests
// }