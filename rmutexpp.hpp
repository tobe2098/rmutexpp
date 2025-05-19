#ifndef _RUST_MUTEX_GUARD_HEADER_
#define _RUST_MUTEX_GUARD_HEADER_

#include <mutex>

template <typename T>
class RMutex{
    mutable std::mutex _internal_mutex;
    T data;
    public:
    template<typename... Args>
    explicit RMutex(Args&&... args) : data(std::forward<Args>(args)...) {}
    RMutex() : data() {}
    // Move constructor
    RMutex(RMutex&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.internal_mutex);
        data = std::move(other.data);
    }

    // Move assignment operator
    RMutex& operator=(RMutex&& other) noexcept {
        if (this != &other) {
            // Lock both mutexes in a consistent order to avoid deadlock
            std::lock(internal_mutex, other.internal_mutex);
            std::lock_guard<std::mutex> lock1(internal_mutex, std::adopt_lock);
            std::lock_guard<std::mutex> lock2(other.internal_mutex, std::adopt_lock);
            data = std::move(other.data);
        }
        return *this;
    }
    // Copy constructor and assignment are deleted to prevent accidental copies
    RMutex(const RMutex&) = delete;
    RMutex& operator=(const RMutex&) = delete;

    // Forward declaration of RMutexGuard
    template<typename U> friend class RMutexGuard;

    // Lock method that returns a guard
    RMutexGuard<T> lock() {
        return RMutexGuard<T>(*this);
    }

    // Const lock method that returns a const guard
    RMutexGuard<const T> lock() const {
        return RMutexGuard<const T>(*this);
    }

    // Try lock method that returns an optional-like wrapper
    // Returns a guard if lock was successful, empty guard otherwise
    RMutexGuard<T> try_lock() {
        return RMutexGuard<T>(*this, std::try_to_lock);
    }

    RMutexGuard<const T> try_lock() const {
        return RMutexGuard<const T>(*this, std::try_to_lock);
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
template <typename... T>
class RMultiMutexGuard {
private:
    std::tuple<RMutex&...> locks_;
    std::tuple<T&...> data_ptrs_;
    bool owns_locks_;

    // Helper to lock all RMutex objects
    template<std::size_t... Is>
    void lock_all(std::index_sequence<Is...>, RMutex<T>&... mutexes) {
        // Lock all mutexes to prevent deadlock
        std::lock(mutexes.get_mutex()...);
        // Adopt the locks into unique_locks
};

// Convenience function for multi-lock with RMutex
template<typename... T>
auto lock_multiple(RMutex<T>&... mutexes) {
    return RMultiMutexGuard<T...>(mutexes...);
}

// Convenience function for multi-try-lock with RMutex
template<typename... T>
auto try_lock_multiple(RMutex<T>&... mutexes) {
    return RMultiMutexGuard<T...>(std::try_to_lock, mutexes...);
}



#endif