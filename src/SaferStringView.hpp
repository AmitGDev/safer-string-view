#ifndef AMITGDEV_SAFERSTRINGVIEW_HPP_
#define AMITGDEV_SAFERSTRINGVIEW_HPP_

/*
    SaferStringView.hpp
    Copyright (c) 2025-2026, Amit Gefen

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include <atomic>
#include <cassert>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace amitgdev {

/**
 * @brief String view that owns rvalues, views lvalues.
 *
 * Prevents dangling references by taking ownership of temporary strings.
 * Lvalue strings must outlive the SaferStringView.
 *
 * A string_view does not carry a null-termination guarantee. Consequently,
 * c_str() materializes an owned string when termination is not known.
 *
 * The valid states of storage_ variant & null_terminated() are:
 *
 *   std::basic_string<T>      & true  - owned, either supplied directly
 *                                       (rvalue string) or materialized
 *                                       lazily by c_str(). Always
 *                                       null-terminated by definition.
 *   std::basic_string_view<T> & true  - borrowed, known null-terminated
 *                                       (e.g., from const T* or lvalue
 *                                       std::basic_string<T>&).
 *   std::basic_string_view<T> & false - borrowed, termination unknown
 *                                       (e.g., from a substring view).
 *
 * The following state is invalid and indicates a bug:
 *
 *   std::basic_string<T>      & false
 *     A std::basic_string<T> is always null-terminated, so the flag must
 *     be true whenever storage_ holds a string.
 *
 * @warning Lifetime information is lost when a string is converted to a
 *          string_view. Consequently, this class cannot protect against a
 *          dangling string_view supplied by the caller. For example:
 *          SaferStringView(std::string_view(std::to_string(2025))) // Dangling!
 *
 * @thread_safety
 * - null_terminated() is lock-free and may be called concurrently with any
 *   operation on the same object.
 * - c_str() is thread-safe with respect to other calls to c_str() and
 *   null_terminated() on the same object.
 * - Copying, moving, assigning, converting to std::basic_string_view, or
 *   destroying a SaferStringView object must not be done concurrently with
 *   c_str() on that object unless externally synchronized.
 *
 * Concurrent mutation of a borrowed string remains the caller's responsibility.
 */
template <typename T>
  requires requires { typename std::char_traits<T>; }
class SaferStringView final {
 public:
  // Non-owning: the string remains owned by the caller.
  explicit SaferStringView(const std::basic_string<T>& str)
      : null_terminated_(true), storage_(std::basic_string_view<T>(str)) {}

  // Owning: take ownership of the temporary string.
  explicit SaferStringView(std::basic_string<T>&& str)
      : null_terminated_(true), storage_(std::move(str)) {}

  // Non-owning: string_view does not guarantee null termination.
  explicit SaferStringView(std::basic_string_view<T> view)
      : null_terminated_(false), storage_(view) {}

  // Non-owning: a valid C string is null-terminated.
  explicit SaferStringView(const T* str)
      : null_terminated_(true), storage_(std::basic_string_view<T>(str)) {}

  // Rule of five: std::atomic and std::mutex are themselves neither copyable
  // nor movable, which would otherwise silently delete these for the whole
  // class. Defined manually so SaferStringView still works as a by-value
  // function parameter, its primary intended use. Each copy/move gets its
  // own mutex; only the logical state (storage_, null_terminated_) transfers.
  SaferStringView(const SaferStringView& other)
      : null_terminated_(
            other.null_terminated_.load(std::memory_order_relaxed)),
        storage_(other.storage_) {
    AssertValidState();
  }

  SaferStringView& operator=(const SaferStringView& other) {
    if (this != &other) {
      storage_ = other.storage_;
      null_terminated_.store(
          other.null_terminated_.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      AssertValidState();
    }

    return *this;
  }

  SaferStringView(SaferStringView&& other) noexcept
      : null_terminated_(
            other.null_terminated_.load(std::memory_order_relaxed)),
        storage_(std::move(other.storage_)) {
    AssertValidState();
  }

  SaferStringView& operator=(SaferStringView&& other) noexcept {
    if (this != &other) {
      storage_ = std::move(other.storage_);
      null_terminated_.store(
          other.null_terminated_.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      AssertValidState();
    }

    return *this;
  }

  ~SaferStringView() = default;

  // Implicit conversion operator for drop-in string_view replacement
  // NOLINTBEGIN(google-explicit-constructor)
  constexpr
  operator std::basic_string_view<T>(this const SaferStringView& self) {
    return std::visit(
        [](const auto& val) -> std::basic_string_view<T> {
          return std::basic_string_view<T>(val);
        },
        self.storage_);
  }

  // NOLINTEND(google-explicit-constructor)

  /**
   * @brief Returns a null-terminated pointer to the string data.
   *
   * If the current representation is not known to be null-terminated, an
   * owned std::basic_string<T> is materialized on first use and reused on
   * subsequent calls.
   *
   * Safe to call concurrently with other calls to c_str() and with
   * null_terminated() on the same object.
   */
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] const T* c_str() const {
    if (!null_terminated_.load(std::memory_order_acquire)) {
      const std::lock_guard lock(materialize_mutex_);

      // Re-check: another thread may have materialized while we waited.
      if (!null_terminated_.load(std::memory_order_relaxed)) {
        const auto* view = std::get_if<std::basic_string_view<T>>(&storage_);
        assert(view != nullptr &&
               "storage_ must hold a string_view when not null terminated");
        storage_ = std::basic_string<T>(*view);
        null_terminated_.store(true, std::memory_order_release);
      }

      return std::visit([](const auto& val) -> const T* { return val.data(); },
                        storage_);
    }

    return std::visit([](const auto& val) -> const T* { return val.data(); },
                      storage_);
  }

  /**
   * @brief Reports whether the current representation is known to be
   * null-terminated.
   */
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] bool null_terminated() const noexcept {
    return null_terminated_.load(std::memory_order_acquire);
  }

 private:
  void AssertValidState() const {
#ifndef NDEBUG
    const bool null_terminated =
        null_terminated_.load(std::memory_order_relaxed);

    if (!null_terminated) {
      assert(
          std::holds_alternative<std::basic_string_view<T>>(storage_) &&
          "A non-null-terminated SaferStringView must contain a string_view");
    }

    if (std::holds_alternative<std::basic_string<T>>(storage_)) {
      assert(null_terminated &&
             "A SaferStringView containing a string must be null-terminated");
    }
#endif  // NDEBUG
  }

  mutable std::atomic<bool> null_terminated_;
  mutable std::mutex materialize_mutex_;

#ifdef TEST_SAFERSTRINGVIEW
 public:
#endif

  mutable std::variant<std::basic_string<T>, std::basic_string_view<T>>
      storage_;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};

}  // namespace amitgdev

#endif  // AMITGDEV_SAFERSTRINGVIEW_HPP_