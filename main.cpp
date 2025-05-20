#include "rmutexpp.hpp"



int main(void){

    RMutex<int> mutex{4};
    RMutexGuard {mutex};
    // RMutexGuard<int>(4);



}