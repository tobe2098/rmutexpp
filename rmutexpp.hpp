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
struct is_rmutex : std::false_type { };

template <typename T>
struct is_rmutex<RMutex<T>> : std::true_type { };

template <typename... Ts>
inline constexpr bool all_are_rmutex = (is_rmutex<Ts>::value && ...);

template <typename>
struct rmutex_data_type;  // no definition: gives an error for invalid types

template <typename T>
struct rmutex_data_type<RMutex<T>> {
    using type = T;
};

// Concept that requires at least 2 template arguments
// template <typename... Ts>
// concept has_multiple_args = requires { requires(sizeof...(Ts) >= 2); };

template <typename T>
using rmutex_data_type_t = typename rmutex_data_type<T>::type;

template <typename... Ts>
  requires(all_are_rmutex<Ts...>)
class RMutexGuard;  // Forward declaration of RMutexGuard template

template <typename T>
  requires all_are_rmutex<T>
class RMutexGuard<T>;

template <typename T>
class RMutex {
    mutable std::mutex _internal_mutex;
    T                  _internal_data;

  public:
    template <typename... Args>
    explicit RMutex(Args&&... args): _internal_data(std::forward<Args>(args)...) { }
    RMutex(): _internal_data() { }
    // Move constructor
    RMutex(RMutex&& other) noexcept {
      std::lock_guard<std::mutex> lock(other._internal_mutex);
      _internal_data = std::move(other._internal_data);
    }

    // Move assignment operator
    RMutex& operator=(RMutex&& other) noexcept {
      if (this != &other) {
        // Lock both mutexes in a consistent order to avoid deadlock
        std::lock(_internal_mutex, other._internal_mutex);
        std::lock_guard<std::mutex> lock1(_internal_mutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(other._internal_mutex, std::adopt_lock);
        _internal_data = std::move(other._internal_data);
      }
      return *this;
    }
    // Copy constructor and assignment are deleted to prevent accidental copies
    RMutex(const RMutex&)            = delete;
    RMutex& operator=(const RMutex&) = delete;

    // Forward declaration of RMutexGuard
    template <typename... Ts>
      requires all_are_rmutex<Ts...>
    friend class RMutexGuard;

    // Lock method that returns a guard
    [[nodiscard]] RMutexGuard<RMutex<T>> lock() { return RMutexGuard(*this); }

    // Const lock method that returns a const guard
    [[nodiscard]] RMutexGuard<RMutex<const T>> lock() const { return RMutexGuard(*this); }

    // Try lock method that returns an optional-like wrapper
    // Returns a guard if lock was successful, empty guard otherwise
    [[nodiscard]] RMutexGuard<RMutex<T>> try_lock() { return RMutexGuard(std::try_to_lock, *this); }

    [[nodiscard]] RMutexGuard<RMutex<const T>> try_lock() const { return RMutexGuard(std::try_to_lock, *this); }
};

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