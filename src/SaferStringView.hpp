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
#include <variant>

/**
 * @brief String view that owns rvalues, views lvalues.
 *
 * Prevents dangling references by taking ownership of temporary strings.
 * Lvalue strings must outlive the SaferStringView.
 *
 * @warning Cannot detect temporaries passed via string_view:
 *          SaferStringView(std::string_view(std::to_string(2025))) // Dangling!
 *
 * @thread_safety
 * c_str() and null_terminated() may be called concurrently on the same
 * SaferStringView object. c_str() is the only operation that guarantees a
 * usable null-terminated pointer.
 *
 * The object must not be copied, moved, assigned, converted to
 * std::basic_string_view, or destroyed concurrently with either operation
 * unless externally synchronized.
 *
 * Other accesses are not synchronized. In particular, concurrent mutation
 * of an lvalue string referenced by this object remains the caller's
 * responsibility.
 */
template <typename T>
  requires requires { typename std::char_traits<T>; }
class SaferStringView final {
 public:
  // Non-owning: from lvalue string reference (stores view)
  explicit SaferStringView(const std::basic_string<T>& str)
      : storage_(std::basic_string_view<T>(str)), null_terminated_(true) {}

  // Owning: from rvalue string (stores owned string)
  explicit SaferStringView(std::basic_string<T>&& str)
      : storage_(std::move(str)), null_terminated_(true) {}

  // Non-owning: from string_view
  explicit SaferStringView(std::basic_string_view<T> view)
      : storage_(view), null_terminated_(false) {}

  // Non-owning: from const char* (common use case)
  explicit SaferStringView(const T* str)
      : storage_(std::basic_string_view<T>(str)), null_terminated_(true) {}

  // Rule of five: std::atomic and std::mutex are themselves neither copyable
  // nor movable, which would otherwise silently delete these for the whole
  // class. Defined manually so SaferStringView still works as a by-value
  // function parameter, its primary intended use. Each copy/move gets its
  // own mutex; only the logical state (storage_, null_terminated_) transfers.
  SaferStringView(const SaferStringView& other)
      : storage_(other.storage_),
        null_terminated_(
            other.null_terminated_.load(std::memory_order_relaxed)) {}

  SaferStringView& operator=(const SaferStringView& other) {
    if (this != &other) {
      storage_ = other.storage_;
      null_terminated_.store(
          other.null_terminated_.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
    }
    return *this;
  }

  SaferStringView(SaferStringView&& other) noexcept
      : storage_(std::move(other.storage_)),
        null_terminated_(
            other.null_terminated_.load(std::memory_order_relaxed)) {}

  SaferStringView& operator=(SaferStringView&& other) noexcept {
    if (this != &other) {
      storage_ = std::move(other.storage_);
      null_terminated_.store(
          other.null_terminated_.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
    }
    return *this;
  }

  ~SaferStringView() = default;

  // Implicit conversion operator for drop-in string_view replacement
  // NOLINTBEGIN(google-explicit-constructor)
  constexpr
  operator std::basic_string_view<T>(this const SaferStringView& self) {
    return std::visit(
        [](const auto& val) -> std::basic_string_view<T> { return val; },
        self.storage_);
  }

  // NOLINTEND(google-explicit-constructor)

  // Guarantees a null-terminated pointer, materializing an owned copy on
  // first use if the value came from a generic string_view with no such
  // guarantee. Subsequent calls reuse the materialized storage.
  //
  // Safe to call concurrently with c_str() and null_terminated() on the
  // same instance. Unlike null_terminated(), this operation guarantees a
  // usable null-terminated pointer.
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

  // Reports whether the current representation is known to be null-terminated.
  //
  // This is informational only and must not be used as a check-then-use
  // substitute. Call c_str() when a null-terminated pointer is required.
  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] bool null_terminated() const noexcept {
    return null_terminated_.load(std::memory_order_acquire);
  }

 private:
  // mutable: c_str() materializes an owned copy and flips this from a
  // const-qualified accessor. load() is const-qualified on std::atomic, but
  // store()/exchange() are not, so mutable is required here just as it is
  // for storage_.
  mutable std::atomic<bool> null_terminated_;
  mutable std::mutex materialize_mutex_;
#ifdef TEST_SAFERSTRINGVIEW
 public:
#endif
  mutable std::variant<std::basic_string<T>, std::basic_string_view<T>>
      storage_;  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};

#endif