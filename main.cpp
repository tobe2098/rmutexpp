#include <iostream>
#include <vector>
#include "rmutex.hpp"
#include "rmutexguard.hpp"
int main(void) {
  // RMutex<const int>         invalid_mutex { 4 };
  // RMutex<RMutex<int>>       invalid_mutex { 4 };
  RMutex<std::string>       mutex { "abs" }, mutex2 {};
  RMutex<std::vector<int>*> guard2 { new std::vector<int> { 1, 2 } };
  RMutexGuard { mutex, mutex2 };
  //   mutex.lock();
  //   mutex2.try_lock();
  {
    const RMutexRef ref = mutex.lock();
    // ref->at(0)          = '2';
  }
  {
    RMutexRef ref  = *mutex.try_lock();
    char&     var  = ((*ref).at(0));
    char&     var2 = ((ref)->at(1));
    std::cout << typeid(var).name() << " " << var << var2 << std::endl;
    unlock((ref));
    // Stale memory
    std::cout << ref->at(0);
    std::cout << ref->at(1);
    std::cout << ref[2];
    mutex.lock();
  }
  // Const ref examples
  {
    // RMutexGuard<RMutex<std::string>, RMutex<std::string>> f2 { mutex, mutex2 };
    RMutexGuard<RMutex<std::string>, RMutex<std::string>> guard { mutex, mutex2 };
    guard.lock();
  }
  //   RMutexGuard guard { mutex, mutex2 };
  // RMutexGuard<int>(4);
}