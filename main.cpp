#include <iostream>
#include "rmutex.hpp"
#include "rmutexguard.hpp"
int main(void) {
  RMutex<std::string> mutex { "abs" }, mutex2 {};
  //   RMutexGuard { mutex, mutex2 };
  //   mutex.lock();
  //   mutex2.try_lock();
  {
    RMutexRef ref  = *mutex.try_lock();
    char&     var  = ((*ref).at(0));
    char&     var2 = ((ref)->at(1));
    std::cout << typeid(var).name() << " " << var << var2 << std::endl;
  }
  RMutexGuard<RMutex<std::string>, RMutex<std::string>> guard { mutex, mutex2 };
  //   RMutexGuard guard { mutex, mutex2 };
  // RMutexGuard<int>(4);
}