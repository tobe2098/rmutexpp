#ifndef _RUST_MUTEX_GUARD_HEADER_
#define _RUST_MUTEX_GUARD_HEADER_

#include <mutex>
#include <tuple>
#include <type_traits>

// Forward declaration so we can use it in the trait
template <typename T>
class RMutex;

// Type trait: is this a specialization of RMutex<T>?
template <typename>
struct is_rmutex : std::false_type {};

template <typename T>
struct is_rmutex<RMutex<T>> : std::true_type {};


template <typename... Ts>
inline constexpr bool all_are_rmutex = (is_rmutex<Ts>::value && ...);

template <typename>
struct rmutex_data_type; // no definition: gives an error for invalid types

template <typename T>
struct rmutex_data_type<RMutex<T>> {
    using type = T;
};

template <typename T>
using rmutex_data_type_t = typename rmutex_data_type<T>::type;

template <typename... Ts>
requires all_are_rmutex<Ts...>
class [[nodiscard]] RMutexGuard ;  // Forward declaration of RMutexGuard template

template <typename T>
class RMutex{
    mutable std::mutex _internal_mutex;
    T data;
    public:
    template<typename... Args>
    explicit RMutex(Args&&... args) : data(std::forward<Args>(args)...) {}
    RMutex(): data() {}
    // Move constructor
    RMutex(RMutex&& other) noexcept {
        std::lock_guard<std::mutex> lock(other._internal_mutex);
        data = std::move(other.data);
    }

    // Move assignment operator
    RMutex& operator=(RMutex&& other) noexcept {
        if (this != &other) {
            // Lock both mutexes in a consistent order to avoid deadlock
            std::lock(_internal_mutex, other._internal_mutex);
            std::lock_guard<std::mutex> lock1(_internal_mutex, std::adopt_lock);
            std::lock_guard<std::mutex> lock2(other._internal_mutex, std::adopt_lock);
            data = std::move(other.data);
        }
        return *this;
    }
    // Copy constructor and assignment are deleted to prevent accidental copies
    RMutex(const RMutex&) = delete;
    RMutex& operator=(const RMutex&) = delete;

    // Forward declaration of RMutexGuard
    template<typename... Ts> requires all_are_rmutex<Ts...> friend class RMutexGuard;

    // Lock method that returns a guard
    [[nodiscard]]RMutexGuard<RMutex<T>> lock()  {
        return RMutexGuard(*this);
    }

    // Const lock method that returns a const guard
    RMutexGuard<RMutex<const T>> lock()const {
        return RMutexGuard(*this);
    }

    // Try lock method that returns an optional-like wrapper
    // Returns a guard if lock was successful, empty guard otherwise
    RMutexGuard<RMutex<T>> try_lock() {
        return RMutexGuard(std::try_to_lock,*this);
    }

    RMutexGuard<RMutex<const T>> try_lock() const {
        return RMutexGuard(std::try_to_lock,*this);
    }
};

// The guard class itself — only enabled for RMutex<T>...
template <typename... Ts>
requires all_are_rmutex<Ts...>
class [[nodiscard]] RMutexGuard {
    //Data members
    std::tuple<Ts&...> locks_; // Each Ts is RMutex<T>
    std::tuple<rmutex_data_type_t<Ts>&...> data_refs_;
    bool owns_locks_;

    // Helper to lock all RMutex objects
    template <std::size_t... Is>
    void lock_all(std::index_sequence<Is...>) {
        // Lock in order to avoid deadlocks
        std::lock(std::get<Is>(locks_)._internal_mutex...);
        owns_locks_ = true;
    }

    template <std::size_t... Is>
    void unlock_all(std::index_sequence<Is...>) {
        if (owns_locks_){
            (std::get<sizeof...(Is) - Is - 1>(locks_).unlock(), ...);
            // Using fold expression (C++17) to unlock in reverse order
        }
    }

    template <std::size_t... Is>
    bool try_lock_all(std::index_sequence<Is...>) {
        bool success = true;

        // Helper array to early exit
        bool locks_acquired[] = {
            (std::get<Is>(locks_)._internal_mutex.try_lock() ? true : (success = false, false))...
        };

        if (!success) {
            // Unlock any that were acquired
            for (std::size_t i = 0; i < sizeof...(Is); ++i) {
                if (locks_acquired[i]) {
                    std::apply([i](auto&... locks_) {
                        std::size_t idx = 0;
                        (..., (idx++ == i ? locks_._internal_mutex.unlock() : void()));
                    }, locks_);
                }
            }
            return false;
        }

        // Store data pointers if success
        owns_locks_ = true;
        return true;
    }


public:
    // Constructor for regular locking of RMutex objects
    explicit RMutexGuard(Ts&... mutexes) : owns_locks_(false),locks_(mutexes...),data_refs_(mutexes.data...) {
        lock_all(std::index_sequence_for<Ts...>{});
    }

    // // Constructor for try locking of RMutex objects
    // RMutexGuard(std::try_to_lock_t, Ts&... mutexes) : owns_locks_(false), locks_(std::forward<Mutexes>(mutexes)...), {
    //     try_lock_all(std::index_sequence_for<Ts...>{});
    // }

    // Default constructor
    RMutexGuard() : owns_locks_(false) {}

    // Destructor
    ~RMutexGuard() = default;

    // Move operations
    RMutexGuard(RMutexGuard&& other) noexcept 
        : locks_(std::move(other.locks_)),
          data_refs_(std::move(other.data_refs_)),
          owns_locks_(other.owns_locks_) {
        other.owns_locks_ = false;
    }

    RMutexGuard& operator=(RMutexGuard&& other) noexcept {
        if (this != &other) {
            locks_ = std::move(other.locks_);
            data_refs_ = std::move(other.data_refs_);
            owns_locks_ = other.owns_locks_;
            other.owns_locks_ = false;
        }
        return *this;
    }

    // Copy operations are deleted
    RMutexGuard(const RMutexGuard&) = delete;
    RMutexGuard& operator=(const RMutexGuard&) = delete;

    // Check if owns all locks
    bool owns_locks() const noexcept { return owns_locks_; }
    operator bool() const noexcept { return owns_locks_; }

    
    // Unlock all mutexes early
    void unlock() {
        if (owns_locks_) {
            std::apply([](auto&... locks) {
                (locks.unlock(), ...);
            }, locks_);
            owns_locks_ = false;
        }
    }

    // // Functional interface - apply function to all references
    // template<typename F>
    // auto apply_all(F&& func) -> decltype(func(std::declval<typename Ts::data_type&...>()...)) {
    //     if (!owns_locks_) {
    //         throw std::runtime_error("Attempting to access data through invalid multi-guard");
    //     }
    //     return std::apply([&func](T*... ptrs) {
    //         return func(*ptrs...);
    //     }, data_refs_);
    // }

    // template<typename F>
    // auto apply_all(F&& func) const -> decltype(func(std::declval<const typename Ts::data_type&...>()...)) {
    //     if (!owns_locks_) {
    //         throw std::runtime_error("Attempting to access data through invalid multi-guard");
    //     }
    //     return std::apply([&func](T*... ptrs) {
    //         return func(*ptrs...);
    //     }, data_refs_);
    // }

    // // Apply function to specific mutex by index
    // template<std::size_t I, typename F>
    // auto apply_at(F&& func) -> decltype(func(std::declval<std::tuple_element_t<I, std::tuple<Ts::data_type&...>>&>())) {
    //     if (!owns_locks_) {
    //         throw std::runtime_error("Attempting to access data through invalid multi-guard");
    //     }
    //     return func(*std::get<I>(data_refs_));
    // }

    // template<std::size_t I, typename F>
    // auto apply_at(F&& func) const -> decltype(func(std::declval<const std::tuple_element_t<I, std::tuple<Ts::data_type&...>>&>())) {
    //     if (!owns_locks_) {
    //         throw std::runtime_error("Attempting to access data through invalid multi-guard");
    //     }
    //     return func(*std::get<I>(data_refs_));
    // }
};

// Convenience function for multi-lock with RMutex
template<typename... T>
auto lock_multiple(RMutex<T>&... mutexes) {
    return RMutexGuard<>(mutexes...);
}

// Convenience function for multi-try-lock with RMutex
template<typename... T>
auto try_lock_multiple(RMutex<T>&... mutexes) {
    return RMutexGuard<>(std::try_to_lock, mutexes...);
}



#endif