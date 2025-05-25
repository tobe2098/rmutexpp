#include <iostream>
#include <vector>
#include "rmutexpp/rmutex.hpp"
#include "rmutexpp/rmutex_guard.hpp"
int main(void) {
  using namespace rmutexpp;
  // rmutexpp::rmutex<const int>         invalid_mutex { 4 };
  // rmutexpp::rmutex<rmutex<int>>       invalid_mutex { 4 };
  rmutexpp::rmutex<std::string>       mutex { "abs" }, mutex2 { "nono" };
  rmutexpp::rmutex<std::vector<int>*> guard2 { new std::vector<int> { 1, 2 } };
  rmutexpp::rmutex_guard { mutex, mutex2 };
  //   mutex.lock();
  //   mutex2.try_lock();
  {
    const rmutexpp::rmutex_ref ref = mutex.lock();
    // ref->at(0)          = '2';
  }
  {
    rmutexpp::rmutex_ref ref  = *mutex.try_lock();
    char&                var  = ((*ref).at(0));
    char&                var2 = ((ref)->at(1));
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
    // rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> f2 { mutex, mutex2 };
    rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> guard { mutex, mutex2 };
    // guard.lock();
    auto [s3, s4] = *guard.get_data();
    s3[0]         = 'z';
    std::cout << s3 << s4 << std::endl;
  }
  {
    // rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> f2 { mutex, mutex2 };
    rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> guard { mutex, mutex2 };
    // guard.lock();
    // std::string s1, s2;
    // std::tie(s1, s2) = *guard.get_data();
    auto [s1, s2] = *guard.get_data();
    std::cout << s1 << s2 << std::endl;
  }
  {
    // rmutex_guard<rmutex<std::string>, rmutex<std::string>> f2 { mutex, mutex2 };
    const rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> guard { mutex, mutex2 };
    // guard.lock();
    // std::string s1, s2;
    // std::tie(s1, s2) = *guard.get_data();
    auto [s1, s2] = *guard.get_data();
    // s1[0]         = 'z';
    std::cout << s1 << s2 << std::endl;
  }
  {
    // rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> f2 { mutex, mutex2 };
    const rmutexpp::rmutex_guard<rmutex<std::string>, rmutex<std::string>> guard { mutex, mutex2 };
    // guard.lock();
    // std::string s1, s2;
    // std::tie(s1, s2) = *guard.get_data();
    auto [s1, s2] = *guard.get_data();
    std::cout << s1 << s2 << std::endl;
  }
  //   rmutexpp::rmutex_guard guard { mutex, mutex2 };
  // rmutexpp::rmutex_guard<int>(4);
}