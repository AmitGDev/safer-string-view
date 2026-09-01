# SaferStringView

A Safe String View Wrapper for C++23 applications (Header-Only Class)

## Overview
* Provides a safe alternative to `std::string_view` that prevents dangling references from temporary strings.
* Automatically takes ownership of rvalue strings while efficiently viewing lvalue strings.
* Drop-in replacement for `std::string_view` with implicit conversion support.
* Leverages C++20 concepts for type safety.
* Zero-overhead abstraction using `std::variant` for storage.
* **C++23 Enhanced:** Uses explicit object parameters (deducing `this`) for cleaner syntax and `constexpr` conversion operators.

## Features
* **Automatic Lifetime Management:**
   * Stores rvalue strings (temporaries) by value to prevent dangling references.
   * Stores lvalue strings as views for efficiency.
   * Safely handles string literals and `const char*` pointers.
* **Drop-in Replacement:**
   * Implicit conversion to `std::string_view` for seamless integration.
   * Works with existing APIs that accept `std::string_view`.
* **Type Safety:**
   * Uses C++20 concepts to ensure only valid character types are used.
   * Explicit constructors prevent accidental conversions.
* **Flexible Construction:**
   * Supports construction from `std::string` (lvalue and rvalue).
   * Supports construction from `std::string_view`.
   * Supports construction from string literals and `const char*`.
* **Memory Efficient:**
   * Only stores owned strings when necessary (rvalue temporaries).
   * Views existing strings without copying when safe.
* **Modern C++23 Syntax:**
   * Uses explicit object parameters (deducing `this`) for improved clarity.
   * `constexpr` conversion operator enables compile-time string operations.

## Usage
- Include the header file:

```cpp
#include "SaferStringView.hpp"
```

- Create SaferStringView instances:

```cpp
// From lvalue string (stores view)
std::string my_string = "Hello World";
SaferStringView ssv1(my_string);

// From rvalue string (takes ownership)
SaferStringView ssv2(std::to_string(2025));

// From string literal (stores view)
SaferStringView ssv3("String Literal");
```

- Use as drop-in replacement for std::string_view:

```cpp
void ProcessString(std::string_view sv) {
  std::cout << "Processing: " << sv << std::endl;
}

SaferStringView ssv(std::to_string(2025));
ProcessString(ssv);  // Implicit conversion to string_view
```

## Problem Solved
Traditional `std::string_view` can lead to dangling references when constructed from temporary strings:

```cpp
// DANGEROUS with std::string_view
std::string_view sv = std::to_string(2025);  // sv now points to destroyed string!
std::cout << sv;  // Undefined behavior!
```

SaferStringView solves this by automatically detecting and storing temporaries:

```cpp
// SAFE with SaferStringView
SaferStringView ssv(std::to_string(2025));  // Owns the string
std::cout << std::string_view(ssv);  // Works correctly!
```

## Technical Details
* **Storage:** Uses `std::variant<std::basic_string<T>, std::basic_string_view<T>>` to store either owned strings or views.
* **Character Types:** Supports any character type with `std::char_traits` (char, wchar_t, char8_t, char16_t, char32_t).
* **Null Termination:** `null_terminated()` reports whether the underlying data is guaranteed null-terminated (true for owned strings, string literals, and `const T*`; false for an arbitrary `std::string_view`).
* **Testing:** Compile with `TEST_SAFERSTRINGVIEW` defined to expose the private `storage_` member for test-only ownership inspection (see `main.cpp`).

## Requirements

A C++23-compatible compiler and standard library.

## Build and Run

- [Build on Windows](https://github.com/AmitGDev/cpp-ci-template#build-on-windows)
- [Build on Linux](https://github.com/AmitGDev/cpp-ci-template#build-on-linux)

## Code Style

- [C++](https://github.com/AmitGDev/cpp-coding-standard)
- [Python](https://github.com/AmitGDev/cpp-ci-template#python)
- [CMake](https://github.com/AmitGDev/cpp-ci-template#cmake)

## License

MIT License. Copyright © 2025-2026 Amit Gefen.

---

**Last Updated:** 2026-10-22  
**Maintainer:** AmitGDev