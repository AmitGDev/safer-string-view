#define TEST_SAFERSTRINGVIEW

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "SaferStringView.hpp"

template <typename T>
static bool OwnsData(const amitgdev::SaferStringView<T>& value) {
  return std::holds_alternative<std::basic_string<T>>(value.storage_);
}

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

static void TestConstructFromLvalueString() {
  const std::string source = "Hello World";

  const amitgdev::SaferStringView value(source);

  assert(std::string_view(value) == source);
  assert(!OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  source: \"" << source << "\"\n"
            << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromRvalueString() {
  const amitgdev::SaferStringView value(std::string("Temporary String"));

  assert(std::string_view(value) == "Temporary String");
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromRvalueFunctionResult() {
  constexpr int kTestNumber = 42;

  const amitgdev::SaferStringView value(std::to_string(kTestNumber));

  assert(std::string_view(value) == "42");
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromStringView() {
  const std::string source = "Base String";
  const std::string_view view(source);

  const amitgdev::SaferStringView value(view);

  assert(std::string_view(value) == view);
  assert(!OwnsData(value));
  assert(!value.null_terminated());

  std::cout << "  source: \"" << source << "\"\n"
            << "  view: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n"
            << "  null-terminated: " << value.null_terminated() << "\n";
}

static void TestConstructFromStringLiteral() {
  const amitgdev::SaferStringView value("String Literal");

  assert(std::string_view(value) == "String Literal");
  assert(!OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromCString() {
  const char* source = "C-style string";

  const amitgdev::SaferStringView value(source);

  assert(std::string_view(value) == source);
  assert(!OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  source: \"" << source << "\"\n"
            << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

// -----------------------------------------------------------------------------
// Copy and move semantics
// -----------------------------------------------------------------------------

static void TestCopyConstruction() {
  const std::string source = "Source";

  const amitgdev::SaferStringView original(source);
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  const amitgdev::SaferStringView copy(original);

  assert(std::string_view(copy) == std::string_view(original));
  assert(!OwnsData(copy));

  std::cout << "  original: \"" << std::string_view(original) << "\"\n"
            << "  copy: \"" << std::string_view(copy) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(copy) << "\n";
}

static void TestMoveConstruction() {
  amitgdev::SaferStringView original(std::string("Source"));

  const amitgdev::SaferStringView moved(std::move(original));

  assert(std::string_view(moved) == "Source");
  assert(OwnsData(moved));

  // Moved-from object is in a valid but unspecified state; do not inspect it.
  std::cout << "  moved: \"" << std::string_view(moved) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(moved) << "\n";
}

static void TestCopyAssignment() {
  amitgdev::SaferStringView destination(std::string("Destination"));
  const amitgdev::SaferStringView source(std::string("Source"));

  destination = source;

  assert(std::string_view(destination) == "Source");
  assert(OwnsData(destination));

  std::cout << "  source: \"" << std::string_view(source) << "\"\n"
            << "  destination: \"" << std::string_view(destination) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(destination) << "\n";
}

static void TestMoveAssignment() {
  amitgdev::SaferStringView destination(std::string("Destination"));
  amitgdev::SaferStringView source(std::string("Source"));

  destination = std::move(source);

  assert(std::string_view(destination) == "Source");
  assert(OwnsData(destination));

  std::cout << "  destination: \"" << std::string_view(destination) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(destination) << "\n";
}

// -----------------------------------------------------------------------------
// std::string_view interoperability
// -----------------------------------------------------------------------------

static void TestStringViewConversion() {
  const amitgdev::SaferStringView value("String View");

  const std::string_view view = value;

  assert(view == "String View");

  std::cout << "  value: \"" << view << "\"\n";
}

// -----------------------------------------------------------------------------
// Null termination
// -----------------------------------------------------------------------------

static void TestNullTerminationForKnownTerminatedInput() {
  const amitgdev::SaferStringView value("Hello");

  assert(value.null_terminated());

  // v1: only verify via string_view conversion
  const std::string_view view = value;
  assert(view == "Hello");

  std::cout << "  value: \"" << view << "\"\n"
            << "  null-terminated: " << std::boolalpha
            << value.null_terminated() << "\n";
}

// -----------------------------------------------------------------------------
// Edge cases
// -----------------------------------------------------------------------------

static void TestEmptyString() {
  const amitgdev::SaferStringView value(std::string(""));

  assert(std::string_view(value).empty());
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestEmptyStringView() {
  const amitgdev::SaferStringView value(std::string_view(""));

  assert(std::string_view(value).empty());
  assert(!OwnsData(value));
  assert(!value.null_terminated());

  std::cout << "  value: \"\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n"
            << "  null-terminated: " << value.null_terminated() << "\n";
}

static void TestEmptyLiteral() {
  const amitgdev::SaferStringView value("");

  assert(std::string_view(value).empty());
  assert(!OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

// -----------------------------------------------------------------------------
// Test suite
// -----------------------------------------------------------------------------

// NOLINTNEXTLINE(bugprone-exception-escape)
int main() {
  const auto run_test = [](const char* name, const auto& test) {
    std::cout << '[' << name << "]\n";
    test();
    std::cout << '\n';
  };

  run_test("ConstructFromLvalueString", TestConstructFromLvalueString);
  run_test("ConstructFromRvalueString", TestConstructFromRvalueString);
  run_test("ConstructFromRvalueFunctionResult",
           TestConstructFromRvalueFunctionResult);
  run_test("ConstructFromStringView", TestConstructFromStringView);
  run_test("ConstructFromStringLiteral", TestConstructFromStringLiteral);
  run_test("ConstructFromCString", TestConstructFromCString);

  run_test("CopyConstruction", TestCopyConstruction);
  run_test("MoveConstruction", TestMoveConstruction);
  run_test("CopyAssignment", TestCopyAssignment);
  run_test("MoveAssignment", TestMoveAssignment);

  run_test("StringViewConversion", TestStringViewConversion);

  run_test("NullTerminationForKnownTerminatedInput",
           TestNullTerminationForKnownTerminatedInput);

  run_test("EmptyString", TestEmptyString);
  run_test("EmptyStringView", TestEmptyStringView);
  run_test("EmptyLiteral", TestEmptyLiteral);

  std::cout << "=== ALL TESTS PASSED ===\n";
}