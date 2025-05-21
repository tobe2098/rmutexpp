#ifndef _RMUTEX_GUARD_HEADER_
#define _RMUTEX_GUARD_HEADER_
#include <tuple>
#include "rmutex.hpp"
// The guard class itself — only enabled for RMutex<T>...
template <typename... Ts>
  requires(all_are_rmutex<Ts...>)
class RMutexGuard {
    // Data members
    std::tuple<Ts&...>                     _mutexes;  // Each Ts is RMutex<T>
    std::tuple<rmutex_data_type_t<Ts>&...> _data_refs;
    bool                                   _owns_locks;

    // Helper to lock all RMutex objects
    template <std::size_t... Is>
    void lock_all(std::index_sequence<Is...>) {
      // Lock in order to avoid deadlocks
      std::lock(std::get<Is>(_mutexes)._internal_mutex...);
      _owns_locks = true;
    }

    template <std::size_t... Is>
    void unlock_all(std::index_sequence<Is...>) {
      if (_owns_locks) {
        (std::get<sizeof...(Is) - Is - 1>(_mutexes).unlock(), ...);
        // Using fold expression (C++17) to unlock in reverse order
      }
    }

    template <std::size_t... Is>
    bool try_lock_all(std::index_sequence<Is...>) {
      return _owns_locks = std::try_lock(std::get<Is>(_mutexes)._internal_mutex...) == -1;
    }

  public:
    // Constructor for regular locking of RMutex objects
    [[nodiscard]] explicit RMutexGuard(Ts&... mutexes): _owns_locks(false), _mutexes(mutexes...), _data_refs(mutexes._internal_data...) {
      lock_all(std::index_sequence_for<Ts...> {});
    }

    // Constructor for try locking of RMutex objects
    RMutexGuard(std::try_to_lock_t, Ts&... mutexes): _owns_locks(false), _mutexes(mutexes...), _data_refs(mutexes._internal_data...) {
      try_lock_all(std::index_sequence_for<Ts...> {});
    }

    // Default constructor
    [[nodiscard]] RMutexGuard(): _owns_locks(false) { }

    // Destructor
    ~RMutexGuard() = default;

    // Move operations
    RMutexGuard(RMutexGuard&& other) noexcept:
        _mutexes(std::move(other._mutexes)), _data_refs(std::move(other._data_refs)), _owns_locks(other._owns_locks) {
      other._owns_locks = false;
    }

    RMutexGuard& operator=(RMutexGuard&& other) noexcept {
      if (this != &other) {
        _mutexes          = std::move(other._mutexes);
        _data_refs        = std::move(other._data_refs);
        _owns_locks       = other._owns_locks;
        other._owns_locks = false;
      }
      return *this;
    }

    // Copy operations are deleted
    RMutexGuard(const RMutexGuard&)            = delete;
    RMutexGuard& operator=(const RMutexGuard&) = delete;

    // Check if owns all locks
    bool     owns_locks() const noexcept { return _owns_locks; }
    explicit operator bool() const noexcept { return _owns_locks; }

    // Unlock all mutexes early
    void unlock() {
      if (_owns_locks) {
        std::apply([](auto&... locks) { (locks.unlock(), ...); }, _mutexes);
        _owns_locks = false;
      }
    }
};
template <typename T>
  requires all_are_rmutex<T>
class RMutexGuard<T> {
    T&                     rmutex;
    rmutex_data_type_t<T>& data_ref;
};

// Deduction guide - allows the compiler to deduce template arguments from constructor arguments
template <typename... Ts>
RMutexGuard(Ts&... args) -> RMutexGuard<Ts...>;

template <typename... Ts>
RMutexGuard(std::try_to_lock_t, Ts&... args) -> RMutexGuard<Ts...>;

#endif