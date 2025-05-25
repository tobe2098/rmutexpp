#ifndef _RUST_MUTEX_GUARD_HEADER_
#define _RUST_MUTEX_GUARD_HEADER_

#include <mutex>
#include <optional>
#include <type_traits>
#ifdef DEBUG
#include <iostream>
#endif
// Forward declaration so we can use the template in the trait
template <typename T>
class RMutex;

/**
 * @struct is_rmutex
 * @brief Type trait to determine if a given type is a specialization of `RMutex`.
 * @tparam U The type to check.
 * @sa is_rmutex<RMutex<T>>
 *
 * This primary template defaults to `std::false_type`, indicating that `U` is not
 * an `RMutex` specialization.
 *
 * @code
 * static_assert(is_rmutex<RMutex<int>>::value, "Should be true");
 * static_assert(!is_rmutex<int>::value, "Should be false");
 * @endcode
 */
template <typename>
struct is_rmutex : std::false_type { };
/**
 * @struct is_rmutex<RMutex<T>>
 * @brief Partial specialization of `is_rmutex` for `RMutex<T>`.
 * @tparam T The data type encapsulated by `RMutex`.
 *
 * This specialization inherits from `std::true_type`, indicating that the
 * checked type `RMutex<T>` is indeed a specialization of `RMutex`.
 */
template <typename T>
struct is_rmutex<RMutex<T>> : std::true_type { };
/**
 * @var all_are_rmutex
 * @brief A convenience variable template to check if all types in a variadic pack are `RMutex` specializations.
 * @tparam Ts A variadic pack of types to check.
 *
 * This variable template is `true` if and only if every type `T` in the `Ts` pack
 * is a specialization of `RMutex<U>` for some `U`.
 *
 * @code
 * static_assert(all_are_rmutex<RMutex<int>, RMutex<std::string>>, "Should be true");
 * static_assert(!all_are_rmutex<RMutex<int>, int>, "Should be false");
 * @endcode
 */
template <typename... Ts>
inline constexpr bool all_are_rmutex = (is_rmutex<Ts>::value && ...);
/**
 * @var is_not_mutex
 * @brief A convenience variable template to check if a type is RMutex<U>.
 * @tparam T A template parameter to check.
 *
 * This variable template is `true` if and only if type T is not of type RMutex<U>.
 *
 * @code
 * static_assert(is_not_mutex<RMutex<int>>, "Should be false");
 * static_assert(!is_not_mutex<int>, "Should be true");
 * @endcode
 */
template <typename T>
inline constexpr bool is_not_mutex = !(is_rmutex<T>::value);
/**
 * @struct rmutex_data_type
 * @brief Type trait to extract the data type `T` from an `RMutex<T>` specialization.
 * @tparam U The `RMutex` specialization (e.g., `RMutex<int>`).
 *
 * This primary template has no definition, resulting in a compile-time error
 * if `U` is not a valid `RMutex` specialization, ensuring correct usage.
 * @sa rmutex_data_type<RMutex<T>>
 */
template <typename>
struct rmutex_data_type;  // no definition: gives an error for invalid types
/**
 * @struct rmutex_data_type<RMutex<T>>
 * @brief Partial specialization of `rmutex_data_type` for `RMutex<T>`.
 * @tparam T The data type encapsulated by `RMutex`.
 *
 * Provides a `using type = T;` member, which aliases the internal data type `T`
 * of the `RMutex` specialization.
 *
 * @code
 * static_assert(std::is_same_v<rmutex_data_type<RMutex<double>>::type, double>, "Should be true");
 * @endcode
 */
template <typename T>
struct rmutex_data_type<RMutex<T>> {
    using type = T;
};
/**
 * @var rmutex_data_type_t
 * @brief Convenience alias template for `rmutex_data_type<U>::type`.
 * @tparam T The `RMutex` specialization.
 *
 * This alias simplifies access to the extracted data type from an `RMutex` specialization.
 *
 * @code
 * using MyDataType = rmutex_data_type_t<RMutex<std::vector<int>>>;
 * // MyDataType is now std::vector<int>
 * @endcode
 */
template <typename T>
using rmutex_data_type_t = typename rmutex_data_type<T>::type;

template <typename T>
struct rmutex_mutex_type;
template <typename T>
struct rmutex_mutex_type<RMutex<T>> {
    using type = std::mutex;
};
template <typename T>
using rmutex_mutex_type_t = typename rmutex_mutex_type<T>::type;
// Forward declaration of RMutexGuard template with a concept/requires clause.
// This indicates that RMutexGuard can only be instantiated with types that are
// all specializations of RMutex.
template <typename... Ts>
  requires(all_are_rmutex<Ts...>)
class RMutexGuard;

// Forward declaration for class friend declaration.
template <typename T>
class RMutexRef;

/**
 * @class RMutex
 * @brief A thread-safe wrapper that protects a single piece of mutable data with a mutex.
 * @tparam T The type of data to be protected.
 *
 * RMutex provides a convenient way to encapsulate a data member with an associated
 * `std::mutex`, ensuring that access to this data is synchronized across multiple threads.
 * It follows the RAII (Resource Acquisition Is Initialization) principle by using
 * `RMutexRef` to manage the lock's lifetime.
 *
 * @note This class explicitly prevents instantiation with `const`-qualified types
 * via a `static_assert`, as mutexes are unnecessary and inefficient for immutable data.
 *
 * @warning `RMutex` is a move-only type; its copy constructor and copy assignment
 * operator are deleted to prevent accidental duplication of the protected resource
 * and potential issues with shared mutex ownership.
 */
template <typename T>
class RMutex {
    // Static assertion to prevent RMutex from being instantiated with a const-qualified type.
    // Mutexes are for mutable data, and using them with const data is inefficient and unnecessary.
    static_assert(!std::is_const<T>::value,
                  "RMutex cannot be instantiated with a const-qualified type (e.g., const int). "
                  "Mutexes are intended for synchronizing access to mutable data. "
                  "If your data is const, no synchronization is needed.");
    static_assert(is_not_mutex<T>, "RMutex cannot contain another RMutex as the underlying type for obvious reasons.");

    std::mutex _internal_mutex;  ///< The underlying mutex protecting _internal_data.

    T _internal_data;  ///< The actual data protected by the mutex.

  public:
    /**
     * @brief Constructs an RMutex object, initializing the protected data with provided arguments.
     * @tparam Args The types of arguments to forward to the underlying data's constructor.
     * @param args Arguments forwarded to the constructor of the internal data (`_internal_data`).
     */
    template <typename... Args>
    explicit RMutex(Args&&... args): _internal_data(std::forward<Args>(args)...) { }
    /**
     * @brief Constructs an RMutex object, value-initializing the protected data.
     * This calls the default constructor of the internal data type `T`.
     */
    RMutex(): _internal_data() { }
    /**
     * @brief Move constructor for RMutex.
     *
     * Moves the protected data from another RMutex object. The mutex of the
     * `other` object is locked during the move operation to ensure thread safety
     * and a consistent state.
     *
     * @param other The RMutex object to move data from.
     */
    RMutex(RMutex&& other) noexcept {
      std::lock_guard<std::mutex> lock(other._internal_mutex);
      _internal_data = std::move(other._internal_data);
    }

    /**
     * @brief Move assignment operator for RMutex.
     *
     * Moves the protected data from another RMutex object to this object.
     * Both mutexes are locked using `std::lock` to prevent deadlocks and
     * ensure atomic transfer of data.
     *
     * @param other The RMutex object to move data from.
     * @return A reference to this RMutex object.
     */
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
    /**
     * @brief Deleted copy constructor.
     * RMutex is a move-only type to prevent accidental copies of the protected data
     * and associated mutex state, which could lead to logical errors or deadlocks.
     */
    RMutex(const RMutex&) = delete;
    /**
     * @brief Deleted copy assignment operator.
     * RMutex is a move-only type.
     */
    RMutex& operator=(const RMutex&) = delete;

    // Friend declarations for RMutexGuard and RMutexRef to allow them to access
    // RMutex's private members (_internal_mutex and _internal_data) for lock management.
    template <typename... Ts>
      requires all_are_rmutex<Ts...>
    friend class RMutexGuard;

    template <typename U>
    friend class RMutexRef;
    /**
     * @brief Acquires a lock on the RMutex and returns an RMutexRef for mutable access.
     *
     * This method blocks until the lock is acquired. The returned `RMutexRef`
     * object ensures the lock is held for its lifetime, providing exclusive,
     * mutable access to the protected data.
     *
     * @return An `RMutexRef<T>` object that manages the lock and provides
     * a mutable reference to the protected data.
     * @sa RMutexRef::operator*(), RMutexRef::operator->(), RMutexRef::operator T&()
     */
    [[nodiscard]] RMutexRef<T> lock() { return RMutexRef(*this); }

    /**
     * @brief Acquires a lock on the RMutex and returns an RMutexRef for const access.
     *
     * This `const`-qualified method can be called on both non-const and const `RMutex`
     * objects. It blocks until the lock is acquired. The returned `RMutexRef<const T>`
     * object ensures the lock is held for its lifetime, providing exclusive,
     * read-only access to the protected data.
     *
     * @return An `RMutexRef<const T>` object that manages the lock and provides
     * a const reference to the protected data.
     * @sa RMutexRef::operator*() const, RMutexRef::operator->() const, RMutexRef::operator const T&() const
     */
    [[nodiscard]] std::optional<RMutexRef<T>> try_lock() { return RMutexRef<T>::try_acquire(*this); }
};
/**
 * @class RMutexRef
 * @brief A RAII-style reference to data protected by an RMutex, providing scoped lock management.
 * @tparam T The type of the data being protected by the RMutex.
 * This cannot be a const-qualified type (e.g., `const int`) because RMutex is enforced to be non-const. Reference constness depends on the
 * user.
 *
 * RMutexRef acts as a smart pointer/reference that, upon construction, acquires a lock on
 * the associated `RMutex` and provides access to its protected data. The lock is
 * automatically released when the `RMutexRef` object goes out of scope (due to its
 * internal `std::unique_lock`'s destructor).
 *
 * This class ensures thread-safe access to data managed by an `RMutex` by strictly
 * enforcing that the mutex remains locked for the duration of the `RMutexRef`'s lifetime.
 *
 * @note RMutexRef is a move-only type, preventing accidental copies that could lead
 * to multiple owners of the same lock and potential deadlocks or incorrect behavior.
 * It also explicitly deletes its copy constructor and assignment operator.
 *
 * @warning Do not manually call `unlock()` or attempt to manage the lock directly
 * through `RMutexRef`. Its purpose is to provide automatic, scoped lock management.
 * For explicit lock attempts that might fail, use `RMutex::try_lock()` which
 * returns an `std::optional<RMutexRef>`.
 */
template <typename T>
class RMutexRef {
    T& data;

    std::unique_lock<std::mutex> _internal_lock;
    /**
     * @brief Private constructor for RMutexRef, used internally to adopt an already acquired lock.
     *
     * This constructor is typically called by static factory methods like `try_acquire`
     * when a lock has been successfully obtained. It takes ownership of an existing
     * `std::unique_lock` and associates it with the protected data reference.
     *
     * @param mutex_data_ref A reference to the data protected by the mutex.
     * @param lock An rvalue-reference to an `std::unique_lock` that has already
     * successfully acquired the mutex. Ownership of this lock is moved
     * into the `RMutexRef` object.
     */
    RMutexRef(T& mutex_data_ref, std::unique_lock<std::mutex>&& lock): data(mutex_data_ref), _internal_lock(std::move(lock)) {
#ifdef DEBUG
      std::cout << "RMutexRef constructed (adopted lock). Type of data: " << typeid(data).name() << std::endl;
#endif
    }

  public:
    /**
     * @brief Constructs an RMutexRef, acquiring a lock on the RMutex.
     *
     * This constructor takes an l-value reference to an RMutex object.
     * The `RMutex<T>&` parameter ensures that only l-value RMutex instances
     * can be passed, preventing construction with temporary RMutex objects.
     *
     * @tparam T The type of data in the RMutex.
     * @param mutex An l-value reference to the RMutex to lock.
     */
    explicit RMutexRef(RMutex<T>& mutex):
        data(mutex._internal_data),            // Access private data (requires friend declaration)
        _internal_lock(mutex._internal_mutex)  // Acquire lock on private mutex
    {
#ifdef DEBUG
      std::cout << "RMutexRef constructed (locked). Type of data: " << typeid(data).name() << std::endl;
#endif
    }
    /**
     * @brief Static factory method to attempt to acquire a lock on an RMutex.
     *
     * If the lock is successfully acquired, an RMutexRef object is returned
     * wrapped in an std::optional. Otherwise, std::nullopt is returned.
     * This prevents the creation of an RMutexRef object when the lock cannot be obtained.
     *
     * @tparam T The type of data in the RMutex.
     * @param mutex An l-value reference to the RMutex to lock.
     * @return An std::optional<RMutexRef<T>> containing the RMutexRef if locked,
     * or std::nullopt if the lock could not be acquired.
     */
    static std::optional<RMutexRef<T>> try_acquire(RMutex<T>& mutex) {
#ifdef DEBUG
      std::cout << "Attempting to acquire lock via try_acquire..." << std::endl;
#endif
      std::unique_lock<std::mutex> lock(mutex._internal_mutex, std::try_to_lock);
      if (lock.owns_lock()) {
#ifdef DEBUG
        std::cout << "  Lock successfully acquired." << std::endl;
#endif
        // Use the private constructor to create an RMutexRef with the adopted lock
        return RMutexRef<T>(mutex._internal_data, std::move(lock));
      } else {
#ifdef DEBUG
        std::cout << "  Failed to acquire lock." << std::endl;
#endif
        return std::nullopt;  // Return empty optional if lock failed
      }
    }
    /**
     * @brief Default destructor for RMutexRef.
     *
     * When an RMutexRef object is destroyed, its owned lock (std::unique_lock)
     * is also destroyed. This automatically releases the underlying mutex,
     * making the protected data available for other threads.
     */
    ~RMutexRef() = default;
    /**
     * @brief Move constructor for RMutexRef.
     *
     * Transfers ownership of the mutex lock from `other` to the newly constructed object.
     * After the move, `other` will no longer own the lock, and the new object
     * will manage the lock and provide access to the same underlying data.
     *
     * @param other The RMutexRef object to move from.
     */
    RMutexRef(RMutexRef<T>&& other) noexcept: _internal_lock(std::move(other._internal_lock)), data(other.data) {
      // Assuming it is locked because cannot unlock via function calls
#ifdef DEBUG
      std::cout << "Move constructor" << std::endl;
#endif
    }

    /**
     * @brief Move assignment operator for RMutexRef.
     *
     * Transfers ownership of the mutex lock from `other` to this object.
     * If this object currently holds a lock, that lock is released before
     * acquiring the lock from `other`. After the assignment, `other`
     * will no longer own its lock.
     *
     * @param other The RMutexRef object to move from.
     * @return A reference to this RMutexRef object.
     */
    RMutexRef<T>& operator=(RMutexRef<T>&& other) noexcept {
      if (this != &other) {
        // std::unique_lock's move assignment handles releasing the current lock
        // and acquiring ownership from 'other'.
        _internal_lock = std::move(other._internal_lock);
        data           = other.data;  // 'data' is a reference, so it refers to the same object
      }
      return *this;
    }
    // Copy constructor and assignment are deleted to prevent deadlocks or accidental releases
    RMutexRef(const RMutexRef<T>&)               = delete;
    RMutexRef<T>& operator=(const RMutexRef<T>&) = delete;
    /**
     * @brief Dereference operator to access the protected data.
     * @return A reference to the protected data.
     */
    T& operator*() & { return data; }
    /**
     * @brief Member access operator to access members of the protected data.
     * @return A pointer to the protected data.
     */
    T* operator->() & { return &data; }
    /**
     * @brief Implicit conversion operator to T&.
     *
     * Allows RMutexRef to be used directly in contexts where a T& is expected,
     * making it behave more like a direct reference. For example, you can pass
     * an RMutexRef object to a function that takes T&.
     *
     * @return A reference to the protected data.
     */
    operator T&() & { return data; }
    /**
     * @brief Dereference operator to access the protected data.
     * @return A reference to the protected data.
     */
    const T& operator*() const& { return data; }
    /**
     * @brief Member access operator to access members of the protected data.
     * @return A pointer to the protected data.
     */
    const T* operator->() const& { return &data; }
    /**
     * @brief Implicit conversion operator to T&.
     *
     * Allows RMutexRef to be used directly in contexts where a T& is expected,
     * making it behave more like a direct reference. For example, you can pass
     * an RMutexRef object to a function that takes T&.
     *
     * @return A reference to the protected data.
     */
    operator const T&() const& { return data; }
    /**
     * @brief Dereference operator to access elements using array-like indexing (non-const).
     *
     * This operator allows direct element access on the protected data `T`
     * using the `[]` syntax, provided `T` itself supports `operator[]`.
     *
     * @param index The index of the element to access.
     * @return A non-const reference to the element at the specified index.
     * @warning This operator is only available if the underlying `T` supports `operator[]`.
     * The behavior for out-of-bounds access depends on the `T` type's `operator[]` implementation.
     */
    decltype(auto) operator[](std::size_t index) { return data[index]; }
};

// Deduction guides - allows the compiler to deduce template arguments from constructor arguments.
// Crucially, these now take an l-value reference to RMutex<T> to enforce l-value binding.

/**
 * @brief Deduction guide for RMutexRef when constructed with a regular lock.
 * @tparam T The type of data in the RMutex.
 * @param arg An l-value reference to the RMutex object.
 * @return RMutexRef<T>
 */
template <typename T>
RMutexRef(RMutex<T>& arg) -> RMutexRef<T>;

/**
 * @brief Deduction guide for RMutexRef when constructed with std::try_to_lock.
 * @tparam T The type of data in the RMutex.
 * @param tag `std::try_to_lock_t`
 * @param arg An l-value reference to the RMutex object.
 * @return RMutexRef<T>
 */
template <typename T>
RMutexRef(std::try_to_lock_t, RMutex<T>& arg) -> RMutexRef<T>;
template <typename T>
[[deprecated("Scope the RMutexRef instead of using unlock()")]] void unlock(RMutexRef<T>& reference) {
  reference.~RMutexRef();
}

#endif