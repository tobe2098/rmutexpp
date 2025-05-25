/**
 * @file rmutex_guard.hpp
 * @brief Defines the RMutexGuard class, a RAII-style lock guard for RMutex objects.
 *
 * This header provides the RMutexGuard class, which manages the locking and
 * unlocking of one or more RMutex instances using a RAII (Resource Acquisition Is Initialization)
 * approach. It supports locking multiple RMutex objects simultaneously to prevent deadlocks
 * and provides access to the protected data when locks are held.
 *
 * @note This file requires 'rmutex.hpp' for RMutex definitions and helper traits.
 */
#ifndef _RMUTEX_GUARD_HEADER_
#define _RMUTEX_GUARD_HEADER_

#include <functional>  // For std::cref
#include <mutex>       // For std::unique_lock, std::lock, std::try_lock, std::defer_lock, std::try_to_lock_t
#include <optional>    // For std::optional
#include <tuple>       // For std::tuple, std::make_tuple, std::get, std::tie
#include <utility>     // For std::move, std::index_sequence, std::index_sequence_for

#include "rmutex.hpp"  // For RMutex, rmutex_mutex_type_t, rmutex_data_type_t, all_are_rmutex

#ifdef DEBUG
#include <iostream>
#endif

/**
 * @class RMutexGuard
 * @brief A RAII-style lock guard for one or more RMutex objects.
 *
 * This primary template for RMutexGuard manages the acquisition and release of
 * locks for multiple RMutex instances. It uses `std::unique_lock` internally
 * and ensures that all specified RMutex objects are locked upon construction
 * and unlocked upon destruction (or move). It supports both regular locking
 * and non-blocking `try_lock` semantics.
 *
 * @tparam Ts A variadic template parameter pack, where each type `T` must be an RMutex instance.
 * @requires all_are_rmutex<Ts...> Ensures that all types in the pack are indeed RMutex types.
 */
template <typename... Ts>
  requires(all_are_rmutex<Ts...>)
class RMutexGuard {
    // Data members
    /// @brief A tuple of unique_lock objects, one for each RMutex.
    /// @note This member is mutable to allow locking operations in const methods.
    mutable std::tuple<std::unique_lock<rmutex_mutex_type_t<Ts>>...> _locks;

    /// @brief A tuple of references to the RMutex objects being guarded.
    std::tuple<Ts&...> _mutex_refs;

    /// @brief A flag indicating whether the guard currently owns the locks.
    /// @note This member is mutable to allow modification by const methods like try_lock_all.
    mutable bool _owns_locks;

    // Helper to lock all RMutex objects
    /**
     * @brief Locks all RMutex objects managed by this guard.
     *
     * This private helper function uses `std::lock` to acquire locks on all
     * mutexes in the `_locks` tuple. `std::lock` is used to prevent deadlocks
     * when locking multiple mutexes.
     *
     * @tparam Is A parameter pack of indices used to iterate over the `_locks` tuple.
     * @param index_sequence A `std::index_sequence` to unpack the indices.
     */
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

    /**
     * @brief Attempts to lock all RMutex objects managed by this guard without blocking.
     *
     * This private helper function uses `std::try_lock` to attempt to acquire
     * locks on all mutexes in the `_locks` tuple. If all locks are acquired,
     * `_owns_locks` is set to true.
     *
     * @tparam Is A parameter pack of indices used to iterate over the `_locks` tuple.
     * @param index_sequence A `std::index_sequence` to unpack the indices.
     * @return True if all locks were successfully acquired, false otherwise.
     */
    template <std::size_t... Is>
    bool try_lock_all(std::index_sequence<Is...>) const& {
      // std::try_lock returns -1 on success, or the index of the mutex that failed to lock.
      return _owns_locks = (std::try_lock(std::get<Is>(_locks)...) == -1);
    }

    /**
     * @brief Creates a tuple of references to the internal data of the guarded RMutex objects.
     *
     * This private helper function is used by `get_data()` to provide access
     * to the protected data of the RMutex instances. This overload provides
     * non-const references.
     *
     * @tparam Is A parameter pack of indices.
     * @param index_sequence A `std::index_sequence` to unpack the indices.
     * @return A `std::tuple` of non-const references to the internal data of the RMutex objects.
     */
    template <std::size_t... Is>
    std::tuple<rmutex_data_type_t<Ts>&...> tuple_of_refs(std::index_sequence<Is...>) & {
      return std::tie((std::get<Is>(_mutex_refs)._internal_data)...);
    }

    /**
     * @brief Creates a tuple of const references to the internal data of the guarded RMutex objects.
     *
     * This private helper function is used by `get_data()` to provide access
     * to the protected data of the RMutex instances. This overload provides
     * const references.
     *
     * @tparam Is A parameter pack of indices.
     * @param index_sequence A `std::index_sequence` to unpack the indices.
     * @return A `std::tuple` of const references to the internal data of the RMutex objects.
     */
    template <std::size_t... Is>
    std::tuple<const rmutex_data_type_t<Ts>&...> tuple_of_refs(std::index_sequence<Is...>) const& {
      return std::tie((std::get<Is>(_mutex_refs)._internal_data)...);
    }

  public:
    /**
     * @brief Constructs an RMutexGuard and locks all provided RMutex objects.
     *
     * This constructor acquires locks on all `mutexes` using `std::lock` to
     * prevent deadlocks. The locks are held until the `RMutexGuard` object
     * goes out of scope or is moved.
     *
     * @param mutexes A variadic list of RMutex objects to be guarded.
     * @pre All `mutexes` must be valid RMutex instances.
     * @post All RMutex objects are locked, and `owns()` returns true.
     */
    [[nodiscard]] explicit RMutexGuard(Ts&... mutexes):
        _owns_locks(false), _locks(std::make_tuple(std::unique_lock<rmutex_mutex_type_t<Ts>>(mutexes._internal_mutex, std::defer_lock)...)),
        _mutex_refs(mutexes...) {
      lock_all(std::index_sequence_for<Ts...> {});
    }

    /**
     * @brief Constructs an RMutexGuard and attempts to lock all provided RMutex objects.
     *
     * This constructor attempts to acquire locks on all `mutexes` using `std::try_lock`.
     * It does not block if any lock cannot be acquired. The `owns()` method
     * can be used to check if all locks were successfully obtained.
     *
     * @param tag A `std::try_to_lock_t` tag to indicate non-blocking try-lock semantics.
     * @param mutexes A variadic list of RMutex objects to be guarded.
     * @pre All `mutexes` must be valid RMutex instances.
     * @post `owns()` reflects whether all RMutex objects were successfully locked.
     */
    [[nodiscard]] RMutexGuard(std::try_to_lock_t tag, Ts&... mutexes):
        _owns_locks(false), _locks(std::make_tuple(std::unique_lock<rmutex_mutex_type_t<Ts>>(mutexes._internal_mutex, std::defer_lock)...)),
        _mutex_refs(mutexes...) {
      try_lock_all(std::index_sequence_for<Ts...> {});
    }

    // Default constructor
    // [[nodiscard]] RMutexGuard(): _owns_locks(false) { }

    /**
     * @brief Destructor for RMutexGuard.
     *
     * When the `RMutexGuard` object is destroyed, it automatically releases
     * any locks it currently owns. This ensures RAII compliance.
     */
    ~RMutexGuard() = default;

    /**
     * @brief Move constructor for RMutexGuard.
     *
     * Transfers ownership of the locks from `other` to the newly constructed object.
     * After the move, `other` no longer owns the locks.
     *
     * @param other The RMutexGuard object to move from.
     */
    RMutexGuard(RMutexGuard&& other) noexcept:
        _locks(std::move(other._locks)), _mutex_refs(std::move(other._mutex_refs)), _owns_locks(other._owns_locks) {
      other._owns_locks = false;
    }

    /**
     * @brief Move assignment operator for RMutexGuard.
     *
     * Transfers ownership of the locks from `other` to this object.
     * Any locks previously held by this object are released. After the move,
     * `other` no longer owns the locks.
     *
     * @param other The RMutexGuard object to move from.
     * @return A reference to this RMutexGuard object.
     */
    RMutexGuard& operator=(RMutexGuard&& other) noexcept {
      if (this != &other) {
        _locks            = std::move(other._locks);
        _mutex_refs       = std::move(other._mutex_refs);
        _owns_locks       = other._owns_locks;
        other._owns_locks = false;
      }
      return *this;
    }

    /// @brief Deleted copy constructor to prevent accidental copying.
    RMutexGuard(const RMutexGuard&) = delete;

    /// @brief Deleted copy assignment operator to prevent accidental copying.
    RMutexGuard& operator=(const RMutexGuard&) = delete;

    /**
     * @brief Checks if the RMutexGuard currently owns all its locks.
     * @return True if all locks are held, false otherwise.
     */
    bool owns() const& noexcept { return _owns_locks; }

    /**
     * @brief Conversion operator to bool.
     * @return True if the RMutexGuard currently owns all its locks, false otherwise.
     */
    explicit operator bool() const& noexcept { return _owns_locks; }

    /// @brief Deleted rvalue reference version of owns() to prevent temporary access.
    bool owns() const&& = delete;

    /// @brief Deleted rvalue reference version of operator bool() to prevent temporary access.
    explicit operator bool() const&& = delete;

    /**
     * @brief Attempts to acquire all locks without blocking.
     *
     * This method can be called on an existing RMutexGuard object that might
     * not currently own its locks (e.g., if constructed with `std::defer_lock`
     * or if a previous `try_lock` failed).
     *
     * @return True if all locks were successfully acquired, false otherwise.
     */
    bool try_lock() const& { return try_lock_all(std::index_sequence_for<Ts...> {}); }

    /**
     * @brief Acquires all locks, blocking if necessary.
     *
     * This method can be called on an existing RMutexGuard object that might
     * not currently own its locks. It will block until all locks are acquired.
     */
    void lock() const& { lock_all(std::index_sequence_for<Ts...> {}); }

    /**
     * @brief Provides access to the guarded data as a tuple of non-const references.
     *
     * This method returns an `std::optional` containing a `std::tuple` of
     * non-const references to the internal data of the RMutex objects,
     * but only if the guard currently owns all its locks.
     *
     * @return An `std::optional` containing a `std::tuple` of non-const references
     * to the data if locks are owned, otherwise `std::nullopt`.
     */
    std::optional<std::tuple<rmutex_data_type_t<Ts>&...>> get_data() & {
      if (_owns_locks) {
        return tuple_of_refs(std::index_sequence_for<Ts...> {});
      } else {
        return std::nullopt;
      }
    }

    /**
     * @brief Provides access to the guarded data as a tuple of const references.
     *
     * This method returns an `std::optional` containing a `std::tuple` of
     * const references to the internal data of the RMutex objects,
     * but only if the guard currently owns all its locks.
     *
     * @return An `std::optional` containing a `std::tuple` of const references
     * to the data if locks are owned, otherwise `std::nullopt`.
     */
    std::optional<std::tuple<const rmutex_data_type_t<Ts>&...>> get_data() const& {
      if (_owns_locks) {
        return tuple_of_refs(std::index_sequence_for<Ts...> {});
      } else {
        return std::nullopt;
      }
    }

    /// @brief Deleted rvalue reference version of get_data() to prevent temporary access.
    std::optional<std::tuple<rmutex_data_type_t<Ts>&...>> get_data() && = delete;

    /// @brief Deleted const rvalue reference version of get_data() to prevent temporary access.
    std::optional<std::tuple<const rmutex_data_type_t<Ts>&...>> get_data() const&& = delete;
};

/**
 * @class RMutexGuard<T>
 * @brief A specialized RAII-style lock guard for a single RMutex object.
 *
 * This specialization of RMutexGuard provides an optimized implementation for
 * managing a single RMutex instance. It behaves similarly to `std::unique_lock`
 * but provides direct access to the RMutex's protected data.
 *
 * @tparam T The type of the single RMutex object to be guarded.
 * @requires all_are_rmutex<T> Ensures that `T` is an RMutex type.
 */
template <typename T>
  requires all_are_rmutex<T>
class RMutexGuard<T> {
    /// @brief The unique_lock object managing the single RMutex.
    /// @note This member is mutable to allow locking operations in const methods.
    mutable std::unique_lock<std::mutex> _lock;

    /// @brief A reference to the internal data of the guarded RMutex.
    rmutex_data_type_t<T>& _data_ref;

    /// @brief A flag indicating whether the guard currently owns the lock.
    /// @note This member is mutable to allow modification by const methods like try_lock.
    mutable bool _owns_lock;

  public:
    /**
     * @brief Constructs an RMutexGuard for a single RMutex and locks it.
     *
     * This constructor acquires a lock on the provided `mutex`. The lock is held
     * until the `RMutexGuard` object goes out of scope or is moved.
     *
     * @param mutex The RMutex object to be guarded.
     * @pre `mutex` must be a valid RMutex instance.
     * @post The RMutex is locked, and `owns()` returns true.
     */
    [[nodiscard]] explicit RMutexGuard(T& mutex): _owns_lock(true), _lock(mutex._internal_mutex), _data_ref(mutex._internal_data) { }

    /**
     * @brief Constructs an RMutexGuard for a single RMutex and attempts to lock it.
     *
     * This constructor attempts to acquire a lock on the provided `mutex` using
     * `std::try_to_lock`. It does not block if the lock cannot be acquired.
     * The `owns()` method can be used to check if the lock was successfully obtained.
     *
     * @param tag A `std::try_to_lock_t` tag to indicate non-blocking try-lock semantics.
     * @param mutex The RMutex object to be guarded.
     * @pre `mutex` must be a valid RMutex instance.
     * @post `owns()` reflects whether the RMutex was successfully locked.
     */
    [[nodiscard]] RMutexGuard(std::try_to_lock_t tag, T& mutex):
        _owns_lock(false), _lock(mutex._internal_mutex, std::try_to_lock), _data_ref(mutex._internal_data) {
      _owns_lock = _lock.owns_lock();
    }

    // Default constructor
    // [[nodiscard]] RMutexGuard(): _owns_locks(false) { }

    /**
     * @brief Destructor for RMutexGuard specialization.
     *
     * When the `RMutexGuard` object is destroyed, it automatically releases
     * the lock it currently owns. This ensures RAII compliance.
     */
    ~RMutexGuard() = default;

    /**
     * @brief Move constructor for RMutexGuard specialization.
     *
     * Transfers ownership of the lock from `other` to the newly constructed object.
     * After the move, `other` no longer owns the lock.
     *
     * @param other The RMutexGuard object to move from.
     */
    RMutexGuard(RMutexGuard&& other) noexcept: _lock(std::move(other._lock)), _data_ref(other._data_ref), _owns_lock(other._owns_lock) {
      other._owns_lock = false;
    }

    /**
     * @brief Move assignment operator for RMutexGuard specialization.
     *
     * Transfers ownership of the lock from `other` to this object.
     * Any lock previously held by this object is released. After the move,
     * `other` no longer owns the lock.
     *
     * @param other The RMutexGuard object to move from.
     * @return A reference to this RMutexGuard object.
     */
    RMutexGuard& operator=(RMutexGuard&& other) noexcept {
      if (this != &other) {
        _lock            = std::move(other._lock);
        _data_ref        = other._data_ref;  // Data reference is copied, not moved, as it refers to external data
        _owns_lock       = other._owns_lock;
        other._owns_lock = false;
      }
      return *this;
    }

    /// @brief Deleted copy constructor to prevent accidental copying.
    RMutexGuard(const RMutexGuard&) = delete;

    /// @brief Deleted copy assignment operator to prevent accidental copying.
    RMutexGuard& operator=(const RMutexGuard&) = delete;

    /**
     * @brief Checks if the RMutexGuard currently owns its lock.
     * @return True if the lock is held, false otherwise.
     */
    bool owns() const& noexcept { return _owns_lock; }

    /**
     * @brief Conversion operator to bool.
     * @return True if the RMutexGuard currently owns its lock, false otherwise.
     */
    explicit operator bool() const& noexcept { return _owns_lock; }

    /// @brief Deleted rvalue reference version of owns() to prevent temporary access.
    bool owns() const&& = delete;

    /// @brief Deleted rvalue reference version of operator bool() to prevent temporary access.
    explicit operator bool() const&& = delete;

    /**
     * @brief Attempts to acquire the lock without blocking.
     *
     * This method can be called on an existing RMutexGuard object that might
     * not currently own its lock.
     *
     * @return True if the lock was successfully acquired, false otherwise.
     */
    bool try_lock() const& { return _owns_lock = _lock.try_lock(); }

    /**
     * @brief Acquires the lock, blocking if necessary.
     *
     * This method can be called on an existing RMutexGuard object that might
     * not currently own its lock. It will block until the lock is acquired.
     */
    void lock() const& {
      _lock.lock();
      _owns_lock = true;
    }

    /**
     * @brief Provides access to the guarded data as a non-const reference.
     *
     * This method returns an `std::optional` containing a non-const reference
     * to the internal data of the RMutex object, but only if the guard
     * currently owns its lock.
     *
     * @return An `std::optional` containing a non-const reference to the data
     * if the lock is owned, otherwise `std::nullopt`.
     */
    std::optional<rmutex_data_type_t<T>&> get_data() & {
      if (_owns_lock) {
        return _data_ref;
      } else {
        return std::nullopt;
      }
    }

    /**
     * @brief Provides access to the guarded data as a const reference.
     *
     * This method returns an `std::optional` containing a const reference
     * to the internal data of the RMutex object, but only if the guard
     * currently owns its lock.
     *
     * @return An `std::optional` containing a const reference to the data
     * if the lock is owned, otherwise `std::nullopt`.
     */
    std::optional<const rmutex_data_type_t<T>&> get_data() const& {
      if (_owns_lock) {
        return std::cref(_data_ref);
      } else {
        return std::nullopt;
      }
    }

    /// @brief Deleted rvalue reference version of get_data() to prevent temporary access.
    std::optional<rmutex_data_type_t<T>&> get_data() && = delete;

    /// @brief Deleted const rvalue reference version of get_data() to prevent temporary access.
    std::optional<const rmutex_data_type_t<T>&> get_data() const&& = delete;
};

/**
 * @brief Deduction guide for RMutexGuard.
 *
 * Allows the compiler to deduce the template arguments `Ts` for `RMutexGuard`
 * when constructing it with a variadic list of RMutex references.
 *
 * @tparam Ts The types of the RMutex objects passed to the constructor.
 * @param args The RMutex objects.
 */
template <typename... Ts>
RMutexGuard(Ts&... args) -> RMutexGuard<Ts...>;

/**
 * @brief Deduction guide for RMutexGuard with `std::try_to_lock_t`.
 *
 * Allows the compiler to deduce the template arguments `Ts` for `RMutexGuard`
 * when constructing it with `std::try_to_lock_t` and a variadic list of RMutex references.
 *
 * @tparam Ts The types of the RMutex objects passed to the constructor.
 * @param tag The `std::try_to_lock_t` tag.
 * @param args The RMutex objects.
 */
template <typename... Ts>
RMutexGuard(std::try_to_lock_t, Ts&... args) -> RMutexGuard<Ts...>;

#endif  // _RMUTEX_GUARD_HEADER_
