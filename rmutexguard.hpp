#ifndef _RMUTEX_GUARD_HEADER_
#define _RMUTEX_GUARD_HEADER_
#include <tuple>
#include "rmutex.hpp"
#ifdef DEBUG
#include <iostream>
#endif
// The guard class itself — only enabled for RMutex<T>...
template <typename... Ts>
  requires(all_are_rmutex<Ts...>)
class RMutexGuard {
    // Data members
    mutable std::tuple<std::unique_lock<rmutex_mutex_type_t<Ts>>...> _locks;
    std::tuple<Ts&...>                                               _mutex_refs;

    mutable bool _owns_locks;

    // Helper to lock all RMutex objects
    template <std::size_t... Is>
    void lock_all(std::index_sequence<Is...>) const& {
      // Lock in order to avoid deadlocks
      std::lock(std::get<Is>(_locks)...);
      _owns_locks = true;
    }

    // template <std::size_t... Is>
    // void unlock_all(std::index_sequence<Is...>) {
    //   if (_owns_locks) {
    //     (std::get<sizeof...(Is) - Is - 1>(_locks).unlock(), ...);
    //     // Using fold expression (C++17) to unlock in reverse order
    //   }
    // }

    template <std::size_t... Is>
    bool try_lock_all(std::index_sequence<Is...>) const& {
      return _owns_locks = (std::try_lock(std::get<Is>(_locks)...) == -1);
    }

    template <std::size_t... Is>
    std::tuple<rmutex_data_type_t<Ts>&...> tuple_of_refs(std::index_sequence<Is...>) & {
      return std::tie((std::get<Is>(_mutex_refs)._internal_data)...);
    }
    template <std::size_t... Is>
    std::tuple<const rmutex_data_type_t<Ts>&...> tuple_of_refs(std::index_sequence<Is...>) const& {
      return std::tie((std::get<Is>(_mutex_refs)._internal_data)...);
    }

  public:
    // Constructor for regular locking of RMutex objects
    [[nodiscard]] explicit RMutexGuard(Ts&... mutexes):
        _owns_locks(false), _locks(std::make_tuple(std::unique_lock<rmutex_mutex_type_t<Ts>>(mutexes._internal_mutex, std::defer_lock)...)),
        _mutex_refs(mutexes...) {
      lock_all(std::index_sequence_for<Ts...> {});
    }

    // Constructor for try locking of RMutex objects
    [[nodiscard]] RMutexGuard(std::try_to_lock_t, Ts&... mutexes):
        _owns_locks(false), _locks(std::make_tuple(std::unique_lock<rmutex_mutex_type_t<Ts>>(mutexes._internal_mutex, std::defer_lock)...)),
        _mutex_refs(mutexes...) {
      try_lock_all(std::index_sequence_for<Ts...> {});
    }

    // Default constructor
    // [[nodiscard]] RMutexGuard(): _owns_locks(false) { }

    // Destructor
    ~RMutexGuard() = default;

    // Move operations
    RMutexGuard(RMutexGuard&& other) noexcept:
        _locks(std::move(other._locks)), _mutex_refs(std::move(other._mutex_refs)), _owns_locks(other._owns_locks) {
      other._owns_locks = false;
    }

    RMutexGuard& operator=(RMutexGuard&& other) noexcept {
      if (this != &other) {
        _locks            = std::move(other._locks);
        _mutex_refs       = std::move(other._mutex_refs);
        _owns_locks       = other._owns_locks;
        other._owns_locks = false;
      }
      return *this;
    }

    // Copy operations are deleted
    RMutexGuard(const RMutexGuard&)            = delete;
    RMutexGuard& operator=(const RMutexGuard&) = delete;

    // Check if owns all locks
    bool     owns() const& noexcept { return _owns_locks; }
    explicit operator bool() const& noexcept { return _owns_locks; }
    bool     owns() const&&          = delete;
    explicit operator bool() const&& = delete;

    bool try_lock() const& { return try_lock_all(std::index_sequence_for<Ts...> {}); }
    void lock() const& { lock_all(std::index_sequence_for<Ts...> {}); }

    std::optional<std::tuple<rmutex_data_type_t<Ts>&...>> get_data() & {
      if (_owns_locks) {
        return tuple_of_refs(std::index_sequence_for<Ts...> {});
      } else {
        return std::nullopt;
      }
    }
    std::optional<std::tuple<const rmutex_data_type_t<Ts>&...>> get_data() const& {
      if (_owns_locks) {
        return tuple_of_refs(std::index_sequence_for<Ts...> {});
      } else {
        return std::nullopt;
      }
    }
    // Explicitly delete rvalue versions to prevent temporary access
    std::optional<std::tuple<rmutex_data_type_t<Ts>&...>>       get_data() &&      = delete;
    std::optional<std::tuple<const rmutex_data_type_t<Ts>&...>> get_data() const&& = delete;
};
template <typename T>
  requires all_are_rmutex<T>
class RMutexGuard<T> {
    mutable std::unique_lock<std::mutex> _lock;
    rmutex_data_type_t<T>&               _data_ref;
    mutable bool                         _owns_lock;

  public:
    // Constructor for regular locking of RMutex objects
    [[nodiscard]] explicit RMutexGuard(T& mutex): _owns_lock(true), _lock(mutex._internal_mutex), _data_ref(mutex._internal_data) { }

    // Constructor for try locking of RMutex objects
    [[nodiscard]] RMutexGuard(std::try_to_lock_t, T& mutex):
        _owns_lock(false), _lock(mutex._internal_mutex, std::try_to_lock), _data_ref(mutex._internal_data) {
      _owns_lock = _lock.owns_lock();
    }

    // Default constructor
    // [[nodiscard]] RMutexGuard(): _owns_locks(false) { }

    // Destructor
    ~RMutexGuard() = default;

    // Move operations
    RMutexGuard(RMutexGuard&& other) noexcept: _lock(std::move(other._lock)), _data_ref(other._data_ref), _owns_lock(other._owns_lock) {
      other._owns_lock = false;
    }

    RMutexGuard& operator=(RMutexGuard&& other) noexcept {
      if (this != &other) {
        _lock            = std::move(other._lock);
        _data_ref        = _data_ref;
        _owns_lock       = other._owns_lock;
        other._owns_lock = false;
      }
      return *this;
    }

    // Copy operations are deleted
    RMutexGuard(const RMutexGuard&)            = delete;
    RMutexGuard& operator=(const RMutexGuard&) = delete;

    // Check if owns all locks
    bool     owns() const& noexcept { return _owns_lock; }
    explicit operator bool() const& noexcept { return _owns_lock; }
    bool     owns() const&&          = delete;
    explicit operator bool() const&& = delete;

    bool try_lock() const& { return _owns_lock = _lock.try_lock(); }
    void lock() const& {
      _lock.lock();
      _owns_lock = true;
    }

    std::optional<rmutex_data_type_t<T>&> get_data() & {
      if (_owns_lock) {
        return _data_ref;
      } else {
        return std::nullopt;
      }
    }
    std::optional<const rmutex_data_type_t<T>&> get_data() const& {
      if (_owns_lock) {
        return std::cref(_data_ref);
      } else {
        return std::nullopt;
      }
    }
    // Explicitly delete rvalue versions to prevent temporary access
    std::optional<rmutex_data_type_t<T>&>       get_data() &&      = delete;
    std::optional<const rmutex_data_type_t<T>&> get_data() const&& = delete;
};

// Deduction guide - allows the compiler to deduce template arguments from constructor arguments
template <typename... Ts>
RMutexGuard(Ts&... args) -> RMutexGuard<Ts...>;

template <typename... Ts>
RMutexGuard(std::try_to_lock_t, Ts&... args) -> RMutexGuard<Ts...>;

#endif