#ifndef AMITG_FC_SAFERSTRINGVIEW_HPP_
#define AMITG_FC_SAFERSTRINGVIEW_HPP_

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

/**
 * @brief String view that owns rvalues and views lvalues.
 *
 * Direct construction from an rvalue std::basic_string transfers ownership.
 * Construction from an lvalue string, string_view, or C string is non-owning.
 *
 * A string_view does not carry a null-termination guarantee. Consequently,
 * c_str() materializes an owned string when termination is not known.
 *
 * @warning Lifetime information is lost when a string is converted to a
 *          string_view. Consequently, this class cannot protect against a
 *          dangling string_view supplied by the caller.
 *
 * @thread_safety
 * c_str() and null_terminated() may be called concurrently on the same
 * SaferStringView object.
 *
 * c_str() always synchronizes access to the internal representation.
 *
 * The object must not be copied, moved, assigned, converted to
 * std::basic_string_view, or destroyed concurrently with either operation
 * unless externally synchronized.
 *
 * Concurrent mutation of a borrowed string remains the caller's responsibility.
 *
 * The valid states are:
 *
 *   std::basic_string<T>      + true  - owned and null-terminated
 *   std::basic_string_view<T> + true  - borrowed and known null-terminated
 *   std::basic_string_view<T> + false - borrowed, termination unknown
 *
 * The following state is invalid:
 *
 *   std::basic_string<T> + false
 */
template <typename T>
  requires requires { typename std::char_traits<T>; }
class SaferStringView final {
 public:
  // Non-owning: the string remains owned by the caller.
  explicit SaferStringView(const std::basic_string<T>& str)
      : storage_(std::basic_string_view<T>(str)), null_terminated_(true) {}

  // Owning: take ownership of the temporary string.
  explicit SaferStringView(std::basic_string<T>&& str)
      : storage_(std::move(str)), null_terminated_(true) {}

  // Non-owning: string_view does not guarantee null termination.
  explicit SaferStringView(std::basic_string_view<T> view)
      : storage_(view), null_terminated_(false) {}

  // Non-owning: a valid C string is null-terminated.
  explicit SaferStringView(const T* str)
      : storage_(std::basic_string_view<T>(str)), null_terminated_(true) {}

  // Each copy receives independent synchronization state.
  SaferStringView(const SaferStringView& other)
      : storage_(other.storage_),
        null_terminated_(
            other.null_terminated_.load(std::memory_order_relaxed)) {
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
      : storage_(std::move(other.storage_)),
        null_terminated_(
            other.null_terminated_.load(std::memory_order_relaxed)) {
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

  // NOLINTBEGIN(google-explicit-constructor)
  constexpr
  operator std::basic_string_view<T>(this const SaferStringView& self) {
    return std::visit(
        [](const auto& value) -> std::basic_string_view<T> {
          return std::basic_string_view<T>(value);
        },
        self.storage_);
  }

  // NOLINTEND(google-explicit-constructor)

  /**
   * @brief Returns a null-terminated representation.
   *
   * If the representation is not known to be null-terminated, an owned string
   * is materialized while holding materialize_mutex_.
   *
   * All access to storage_ performed by this function is protected by
   * materialize_mutex_.
   */
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] const T* c_str() const {
    const std::lock_guard lock(materialize_mutex_);

    if (!null_terminated_.load(std::memory_order_relaxed)) {
      const auto* view = std::get_if<std::basic_string_view<T>>(&storage_);

      assert(view != nullptr &&
             "non-null-terminated storage must contain a string_view");

      storage_ = std::basic_string<T>(*view);
      null_terminated_.store(true, std::memory_order_relaxed);
    }

    return std::visit(
        [](const auto& value) -> const T* { return value.data(); }, storage_);
  }

  /**
   * @brief Reports whether the current representation is known to be
   * null-terminated.
   *
   * This is informational only and must not be used as a check-then-use
   * substitute. Call c_str() when a null-terminated pointer is required.
   */
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] bool null_terminated() const noexcept {
    return null_terminated_.load(std::memory_order_acquire);
  }

 private:
  void AssertValidState() const {
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
  }

  mutable std::atomic<bool> null_terminated_;
  mutable std::mutex materialize_mutex_;

#ifdef TEST_SAFERSTRINGVIEW
 public:
#endif

  mutable std::variant<std::basic_string<T>, std::basic_string_view<T>>
      storage_;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};

#endif  // AMITG_FC_SAFERSTRINGVIEW_HPP_
