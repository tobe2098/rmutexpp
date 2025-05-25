## rmutex and rmutex_guard: Rust-Inspired Mutexes for C++

This library provides `rmutex`,`rmutex_ref` and `rmutex_guard`, C++ implementations inspired by Rust's robust concurrency primitives. Designed for thread-safe data access, these classes leverage RAII (Resource Acquisition Is Initialization) to simplify mutex management and help prevent common concurrency bugs like deadlocks and data races.

[![C++ CI/CD (Build & Test)](https://github.com/tobe2098/rmutexpp/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/tobe2098/rmutexpp/actions/workflows/build_and_test.yml)
---

Go [here](./Install.md) for installation instructions.

### Key Components

#### `rmutex<T>`: The Thread-Safe Data Container

`rmutex<T>` is a wrapper that protects a single piece of mutable data (`T`) with an associated `std::mutex`. It ensures that all access to the encapsulated data is synchronized, making it safe to share across multiple threads.

**Key Features of `rmutex`:**

* **RAII for Data Protection**: Manages an internal `std::mutex` that automatically locks and unlocks access to `_internal_data`.
* **Move-Only**: `rmutex` instances are move-only, preventing accidental copying of the mutex and its protected data, which can lead to complex concurrency issues.
* **No `const T`**: `rmutex` cannot be instantiated with `const` types, as mutexes are unnecessary and inefficient for immutable data.
* **No Nested `rmutex`**: Prevents `rmutex<rmutex<T>>` to avoid confusing and potentially problematic nested locking scenarios.

**Example Usage:**

```cpp
rmutex<std::string> myMutexString { "Hello, rmutex!" };
rmutex<std::vector<int>> myMutexVector { {10, 20, 30} };
```

---

#### `rmutex_ref<T>`: Scoped Access to `rmutex` Data

`rmutex_ref<T>` is a RAII-style smart reference that provides scoped, thread-safe access to the data inside an `rmutex<T>`. When you `lock()` an `rmutex`, it returns an `rmutex_ref` which holds the lock. The lock is automatically released when the `rmutex_ref` goes out of scope.

**Key Features of `rmutex_ref`:**

* **Automatic Locking/Unlocking**: Acquires the mutex lock upon construction and releases it upon destruction, ensuring data integrity.
* **Direct Data Access**: Overloads `operator*`, `operator->`, and `operator[]` (if applicable to `T`) to provide intuitive access to the protected data. It also has an implicit conversion to `T&`.
* **Move-Only**: Like `rmutex`, `rmutex_ref` is move-only to enforce clear ownership of the lock.
* **`try_acquire` for Non-Blocking Lock Attempts**: Use `rmutex_ref::try_acquire(mutex)` to attempt to acquire the lock without blocking. This returns an `std::optional<rmutex_ref<T>>`.

**Example Usage:**

```cpp
rmutex<std::string> mutex { "initial" };

// Acquire a lock and get mutable access
{
    rmutex_ref<std::string> ref = mutex.lock(); // Blocks until lock is acquired
    *ref = "modified";
    std::cout << "Inside rmutex_ref scope (mutable): " << *ref << std::endl;
} // Lock is automatically released here

// Attempt to acquire a lock (non-blocking) and get mutable access
if (auto optionalRef = rmutex_ref<std::string>::try_acquire(mutex)) {
    rmutex_ref<std::string>& ref = *optionalRef; // Dereference the optional
    std::cout << "Inside rmutex_ref scope (try_acquire, mutable): " << ref->length() << std::endl;
    ref->append(" and appended.");
}

// Acquire a lock and get const access
{
    const rmutex_ref<std::string> constRef = mutex.lock();
    std::cout << "Inside rmutex_ref scope (const): " << *constRef << std::endl;
    // *constRef = "cannot modify"; // This would be a compile-time error
} // Lock is automatically released here
```

---

#### `rmutex_guard<Ts...>`: Locking Multiple `rmutex` Objects

`rmutex_guard` is a powerful RAII construct for managing locks on **one or more** `rmutex` objects simultaneously. It uses `std::lock` internally when locking multiple mutexes to prevent deadlocks.

**Key Features of `rmutex_guard`:**

* **Deadlock Prevention**: Uses `std::lock` to acquire multiple mutexes atomically, ensuring they are all locked or none are, preventing classic deadlock scenarios.
* **Variadic Template**: Can guard any number of `rmutex` instances.
* **Access to Protected Data**: Provides `get_data()` methods that return an `std::optional<std::tuple<T1&, T2&...>>` (or const references) to the protected data, allowing structured binding for easy access.
* **Move-Only**: `rmutex_guard` is move-only to maintain clear ownership of the managed locks.
* **Constructors for Blocking and Non-Blocking Locks**:
    * `rmutex_guard(mutex1, mutex2, ...)`: Blocks until all mutexes are locked.
    * `rmutex_guard(std::try_to_lock_t, mutex1, mutex2, ...)`: Attempts to lock all mutexes without blocking; check `owns()` afterwards.

**Example Usage:**

```cpp
rmutex<std::string> mutex1 { "First" };
rmutex<int> mutex2 { 123 };

// Lock multiple mutexes simultaneously
{
    rmutex_guard guard { mutex1, mutex2 }; // Locks both mutex1 and mutex2
    if (guard.owns()) {
        auto [s1, i1] = *guard.get_data(); // Structured binding for easy access
        s1[0] = 'z'; // Modify data in mutex1
        i1++;        // Modify data in mutex2
        std::cout << "Guarded data: " << s1 << ", " << i1 << std::endl;
    }
} // Both mutexes are automatically unlocked here

// Using try_to_lock with rmutex_guard
{
    rmutex_guard tryGuard { std::try_to_lock, mutex1, mutex2 };
    if (tryGuard.owns()) {
        auto [s1, i1] = *tryGuard.get_data();
        std::cout << "Successfully acquired all locks via try_lock: " << s1 << ", " << i1 << std::endl;
    } else {
        std::cout << "Failed to acquire all locks via try_lock." << std::endl;
    }
}
```

---

### Important Considerations and Idioms

* **RAII is King**: Always prefer using `rmutex_ref` or `rmutex_guard` to manage locks. Manual `lock()` and `unlock()` calls on the raw `std::mutex` are discouraged as they are error-prone.
* **Avoid Manual `unlock()`**: The `unlock` function from the library is `[[deprecated]]` because it breaks the RAII pattern and can lead to immediate undefined behavior if the `rmutex_ref` is used after being "unlocked" this way.
* **L-Value References for `rmutex`**: Constructors and deduction guides for `rmutex_ref` and `rmutex_guard` explicitly require l-value references to `rmutex` objects. This prevents guarding temporary `rmutex` instances, ensuring proper lifecycle management.
* **No Const `rmutex`**: You cannot declare `const rmutex<T>`. If your data is truly `const`, it doesn't need a mutex.
* **Structured Bindings**: When using `rmutex_guard::get_data()`, leverage C++17 structured bindings (`auto [var1, var2] = *guard.get_data();`) for clean and concise access to multiple protected data elements.

This library aims to provide a safer, more intuitive way to handle concurrency in C++ by encapsulating data and its synchronization mechanism in a robust, Rust-inspired manner.