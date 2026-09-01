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

  // Implicit conversion operator for drop-in string_view replacement
  // NOLINTBEGIN(google-explicit-constructor)
  constexpr
  operator std::basic_string_view<T>(this const SaferStringView& self) {
    return std::visit(
        [](const auto& val) -> std::basic_string_view<T> { return val; },
        self.storage_);
  }

  // NOLINTEND(google-explicit-constructor)

  // NOLINTNEXTLINE(readability-identifier-naming)
  [[nodiscard]] bool null_terminated() const noexcept {
    return null_terminated_;
  }

 private:
  bool null_terminated_;
#ifdef TEST_SAFERSTRINGVIEW
 public:
#endif
  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  std::variant<std::basic_string<T>, std::basic_string_view<T>> storage_;
};

#endif