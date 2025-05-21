#ifndef _RUST_MUTEX_GUARD_HEADER_
#define _RUST_MUTEX_GUARD_HEADER_

#include <mutex>
#include <optional>
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

template <typename T>
using rmutex_data_type_t = typename rmutex_data_type<T>::type;

template <typename... Ts>
  requires(all_are_rmutex<Ts...>)
class RMutexGuard;  // Forward declaration of RMutexGuard template

template <typename T>
class RMutexRef;

// Forward declaration of the static try_acquire method.
// This allows other functions or classes to call it before the full
// definition of RMutexRef is available.
// template <typename U>
// std::optional<RMutexRef<U>> RMutexRef<U>::try_acquire(RMutex<U>& mutex);

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

    template <typename U>
    friend class RMutexRef;
    // Lock method that returns a guard
    [[nodiscard]] RMutexRef<T> lock() { return RMutexRef(*this); }

    // Const lock method that returns a const guard
    [[nodiscard]] RMutexRef<const T> lock() const { return RMutexRef(*this); }

    // Try lock method that returns an optional-like wrapper
    // Returns a guard if lock was successful, empty guard otherwise
    [[nodiscard]] std::optional<RMutexRef<T>> try_lock() { return RMutexRef<T>::try_acquire(*this); }

    [[nodiscard]] std::optional<RMutexRef<const T>> try_lock() const { return RMutexRef<T>::try_acquire(*this); }
};

template <typename T>
class RMutexRef {
    T& data;

    std::unique_lock<std::mutex> _internal_lock;

    RMutexRef(T& mutex_data_ref, std::unique_lock<std::mutex>&& lock): data(mutex_data_ref), _internal_lock(std::move(lock)) {
      std::cout << "RMutexRef constructed (adopted lock). Type of data: " << typeid(data).name() << std::endl;
    }

  public:
    /**
     * @brief Constructs an RMutexRef, acquiring a lock on the RMutex.
     *
     * This constructor takes an l-value reference to an RMutex object.
     * The `RMutex<U>&` parameter ensures that only l-value RMutex instances
     * can be passed, preventing construction with temporary RMutex objects.
     *
     * @tparam U The type of data in the RMutex.
     * @param mutex An l-value reference to the RMutex to lock.
     */
    explicit RMutexRef(RMutex<T>& mutex):
        data(mutex._internal_data),            // Access private data (requires friend declaration)
        _internal_lock(mutex._internal_mutex)  // Acquire lock on private mutex
    {
      std::cout << "RMutexRef constructed (locked). Type of data: " << typeid(data).name() << std::endl;
    }
    /**
     * @brief Static factory method to attempt to acquire a lock on an RMutex.
     *
     * If the lock is successfully acquired, an RMutexRef object is returned
     * wrapped in an std::optional. Otherwise, std::nullopt is returned.
     * This prevents the creation of an RMutexRef object when the lock cannot be obtained.
     *
     * @tparam U The type of data in the RMutex.
     * @param mutex An l-value reference to the RMutex to lock.
     * @return An std::optional<RMutexRef<U>> containing the RMutexRef if locked,
     * or std::nullopt if the lock could not be acquired.
     */
    static std::optional<RMutexRef<T>> try_acquire(RMutex<T>& mutex) {
      std::cout << "Attempting to acquire lock via try_acquire..." << std::endl;
      std::unique_lock<std::mutex> lock(mutex._internal_mutex, std::try_to_lock);
      if (lock.owns_lock()) {
        std::cout << "  Lock successfully acquired." << std::endl;
        // Use the private constructor to create an RMutexRef with the adopted lock
        return RMutexRef<T>(mutex._internal_data, std::move(lock));
      } else {
        std::cout << "  Failed to acquire lock." << std::endl;
        return std::nullopt;  // Return empty optional if lock failed
      }
    }
    ~RMutexRef() = default;
    // Move constructor
    RMutexRef(RMutexRef<T>&& other) noexcept: _internal_lock(std::move(other._internal_lock)), data(other.data) {
      // Assuming it is locked because cannot unlock via function calls
    }

    // Move assignment operator
    RMutexRef<T>& operator=(RMutexRef<T>&& other) noexcept {
      if (this != &other) {
        // Release our mutex lock via move
        _internal_lock = std::move(other._internal_lock);
        data           = other.data;
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

#endif