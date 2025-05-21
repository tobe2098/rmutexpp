#include "rmutexpp.hpp"

int main(void) {
  RMutex<int> mutex { 4 }, mutex2 {};

  //   RMutexGuard { mutex, mutex2 };
  //   mutex.lock();
  //   mutex2.try_lock();
  RMutexGuard<RMutex<int>, RMutex<int>> guard { mutex, mutex2 };
  //   RMutexGuard guard { mutex, mutex2 };

  // RMutexGuard<int>(4);
}