#ifndef _RUST_MUTEX_GUARD_HEADER_
#define _RUST_MUTEX_GUARD_HEADER_

#include <mutex>

template <typename T>
class RMutex{

    // Move assignment operator
    RMutex& operator=(RMutex&& other) noexcept {
    }
    // Copy constructor and assignment are deleted to prevent accidental copies
    RMutex(const RMutex&) = delete;
    RMutex& operator=(const RMutex&) = delete;

    // Forward declaration of RMutexGuard
    template<typename U> friend class RMutexGuard;

    // Lock method that returns a guard
    RMutexGuard<T> lock() {
    }

    // Const lock method that returns a const guard
    RMutexGuard<const T> lock() const {
    }

    // Try lock method that returns an optional-like wrapper
    // Returns a guard if lock was successful, empty guard otherwise
    RMutexGuard<T> try_lock() {
    }

    RMutexGuard<const T> try_lock() const {
    }
};

template <typename T>
class RMutexGuard{
    std::mutex _internal_mutex;
    T data;
    public:
    RMutexGuard(){
        
    }
    ~RMutexGuard(){}
    RMutexGuard(RMutexGuard&&){}
    operator=(RMutexGuard&&){}
    RMutexGuard(RMutexGuard&)=delete;
    operator=(RMutexGuard&)=delete;
};



#endif