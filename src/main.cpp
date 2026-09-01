#define TEST_SAFERSTRINGVIEW

#include <cassert>
#include <iostream>
#include <latch>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "SaferStringView.hpp"

template <typename T>
static bool OwnsData(const SaferStringView<T>& value) {
  return std::holds_alternative<std::basic_string<T>>(value.storage_);
}

[[nodiscard]] static bool ConsumeStringView(std::string_view value) {
  return !value.empty();
}

[[nodiscard]] static std::string ConsumeCString(const char* value) {
  return value;
}

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

static void TestConstructFromLvalueString() {
  const std::string source = "Hello World";

  const SaferStringView value(source);

  assert(std::string_view(value) == source);
  assert(!OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  source: \"" << source << "\"\n"
            << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromRvalueString() {
  const SaferStringView value(std::string("Temporary String"));

  assert(std::string_view(value) == "Temporary String");
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromRvalueFunctionResult() {
  constexpr int kTestNumber = 42;

  const SaferStringView value(std::to_string(kTestNumber));

  assert(std::string_view(value) == "42");
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromStringView() {
  const std::string source = "Base String";
  const std::string_view view(source);

  const SaferStringView value(view);

  assert(std::string_view(value) == view);
  assert(!OwnsData(value));
  assert(!value.null_terminated());

  std::cout << "  source: \"" << source << "\"\n"
            << "  view: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n"
            << "  null-terminated: " << value.null_terminated() << "\n";
}

static void TestConstructFromStringLiteral() {
  const SaferStringView value("String Literal");

  assert(std::string_view(value) == "String Literal");
  assert(!OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestConstructFromCString() {
  const char* source = "C-style string";

  const SaferStringView value(source);

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

  const SaferStringView original(source);
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  const SaferStringView copy(original);

  assert(std::string_view(copy) == std::string_view(original));
  assert(!OwnsData(copy));

  std::cout << "  original: \"" << std::string_view(original) << "\"\n"
            << "  copy: \"" << std::string_view(copy) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(copy) << "\n";
}

static void TestMoveConstruction() {
  SaferStringView original(std::string("Source"));

  const SaferStringView moved(std::move(original));

  assert(std::string_view(moved) == "Source");
  assert(OwnsData(moved));

  std::cout << "  moved: \"" << std::string_view(moved) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(moved) << "\n";
}

static void TestCopyAssignment() {
  SaferStringView destination(std::string("Destination"));
  const SaferStringView source(std::string("Source"));

  destination = source;

  assert(std::string_view(destination) == "Source");
  assert(OwnsData(destination));

  std::cout << "  source: \"" << std::string_view(source) << "\"\n"
            << "  destination: \"" << std::string_view(destination) << "\"\n"
            << "  owns: " << std::boolalpha << OwnsData(destination) << "\n";
}

static void TestMoveAssignment() {
  SaferStringView destination(std::string("Destination"));
  SaferStringView source(std::string("Source"));

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
  const SaferStringView value("String View");

  const std::string_view view = value;

  assert(view == "String View");

  std::cout << "  value: \"" << view << "\"\n";
}

static void TestStringViewConsumer() {
  const SaferStringView value("String View");

  assert(ConsumeStringView(value));

  std::cout << "  consumed: \"" << std::string_view(value) << "\"\n";
}

// -----------------------------------------------------------------------------
// Null termination
// -----------------------------------------------------------------------------

static void TestNullTerminationForKnownTerminatedInput() {
  const SaferStringView value("Hello");

  assert(value.null_terminated());

  const char* pointer = value.c_str();

  assert(pointer != nullptr);

  const std::string_view c_string(pointer);
  assert(c_string == "Hello");

  std::cout << "  value: \"" << std::string_view(value) << "\"\n"
            << "  null-terminated: " << std::boolalpha
            << value.null_terminated() << "\n"
            << "  c_str(): \"" << c_string << "\"\n";
}

static void TestMaterializesNonTerminatedView() {
  const std::string source = "prefix|substring|suffix";
  const std::string_view substring = std::string_view(source).substr(7, 9);

  assert(substring == "substring");

  const SaferStringView value(substring);

  assert(!value.null_terminated());
  assert(!OwnsData(value));

  std::cout << "  source: \"" << source << "\"\n"
            << "  view: \"" << substring << "\"\n"
            << "  before c_str(): owns=" << std::boolalpha << OwnsData(value)
            << ", null-terminated=" << value.null_terminated() << "\n";

  const char* first = value.c_str();

  assert(value.null_terminated());
  assert(OwnsData(value));

  const std::string_view materialized(first);

  assert(materialized == substring);
  assert(materialized.size() == substring.size());
  assert(ConsumeCString(first) == "substring");

  const char* second = value.c_str();

  assert(second == first);
  assert(std::string_view(second) == "substring");
  // The implicit conversion must now read the materialized owned string,
  // not the original (now-superseded) view into `source`.
  assert(std::string_view(value) == "substring");

  std::cout << "  after c_str(): owns=" << std::boolalpha << OwnsData(value)
            << ", null-terminated=" << value.null_terminated() << "\n"
            << "  c_str(): \"" << materialized << "\"\n"
            << "  pointer reused: " << (first == second) << "\n";
}

static void TestMaterializesEmptyView() {
  const std::string source = "non-empty";
  const std::string_view empty_view = std::string_view(source).substr(4, 0);

  const SaferStringView value(empty_view);

  assert(!value.null_terminated());
  assert(!OwnsData(value));

  const char* pointer = value.c_str();

  assert(pointer != nullptr);
  assert(*pointer == '\0');
  assert(OwnsData(value));
  assert(value.null_terminated());

  const std::string_view materialized(pointer);

  assert(materialized.empty());
  assert(pointer != nullptr);
  assert(*pointer == '\0');

  std::cout << "  before c_str(): owns=false, null-terminated=false\n"
            << "  after c_str(): owns=true, null-terminated=true\n"
            << "  c_str(): \"\"\n";
}

// -----------------------------------------------------------------------------
// Concurrency
// -----------------------------------------------------------------------------

// Validates the documented contract that c_str() and null_terminated() may
// be called concurrently on the same instance, and that materialization
// happens exactly once even under contention: every thread must observe the
// same materialized pointer and correct content. The interleaving
// itself is non-deterministic, but the asserted outcome is not.
static void TestConcurrentCStr() {
  const std::string source = "prefix|concurrent-substring|suffix";
  const std::string_view substring = std::string_view(source).substr(7, 20);

  assert(substring == "concurrent-substring");

  const SaferStringView value(substring);

  assert(!value.null_terminated());

  constexpr int kThreadCount = 8;
  std::latch start_line(kThreadCount);
  std::vector<const char*> results(kThreadCount, nullptr);
  std::vector<std::thread> threads;
  threads.reserve(kThreadCount);

  for (const int index : std::views::iota(0, kThreadCount)) {
    threads.emplace_back([&value, &start_line, &results, index] {
      // Maximize contention on the not-yet-materialized fast-path check.
      start_line.arrive_and_wait();
      results.at(index) = value.c_str();
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  assert(value.null_terminated());

  const char* const expected = results.front();

  assert(expected != nullptr);
  assert(std::string_view(expected) == "concurrent-substring");

  bool all_pointers_equal = true;

  for (const char* result : results) {
    all_pointers_equal &= result == expected;
  }

  assert(all_pointers_equal);

  assert(all_pointers_equal);

  std::cout << "  source: \"" << source << "\"\n"
            << "  view: \"" << substring << "\"\n"
            << "  threads: " << kThreadCount << "\n"
            << "  all pointers equal: " << std::boolalpha << all_pointers_equal
            << "\n"
            << "  c_str(): \"" << expected << "\"\n";
}

// -----------------------------------------------------------------------------
// Wide-character instantiation
// -----------------------------------------------------------------------------

// SaferStringView<wchar_t> is the primary motivating case for c_str():
// WinAPI parameters require a null-terminated wchar_t string, which a
// std::wstring_view does not guarantee.
static void TestWideCharInstantiation() {
  const std::wstring source = L"prefix|wide substring|suffix";
  const std::wstring_view substring = std::wstring_view(source).substr(7, 14);

  assert(substring == L"wide substring");

  const SaferStringView<wchar_t> value(substring);

  assert(!value.null_terminated());
  assert(!OwnsData(value));

  const wchar_t* pointer = value.c_str();

  assert(value.null_terminated());
  assert(OwnsData(value));
  assert(pointer != nullptr);

  const std::wstring_view materialized(pointer);

  assert(materialized.size() == substring.size());
  assert(materialized == substring);
  assert(std::wstring_view(value) == substring);

  std::wcout << L"  source: \"" << source << L"\"\n"
             << L"  view: \"" << substring << L"\"\n"
             << L"  c_str(): \"" << materialized << L"\"\n";
}

// -----------------------------------------------------------------------------
// Edge cases
// -----------------------------------------------------------------------------

static void TestEmptyString() {
  const SaferStringView value(std::string(""));

  assert(std::string_view(value).empty());
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  value: \"\"\n"
            << "  owns: " << std::boolalpha << OwnsData(value) << "\n";
}

static void TestEmptyStringView() {
  const SaferStringView value(std::string_view(""));

  assert(std::string_view(value).empty());
  assert(!OwnsData(value));
  assert(!value.null_terminated());

  const char* pointer = value.c_str();

  assert(pointer != nullptr);
  assert(*pointer == '\0');
  assert(OwnsData(value));
  assert(value.null_terminated());

  std::cout << "  before c_str(): owns=false, null-terminated=false\n"
            << "  after c_str(): owns=true, null-terminated=true\n"
            << "  c_str(): \"" << pointer << "\"\n";
}

static void TestEmptyLiteral() {
  const SaferStringView value("");

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
  run_test("StringViewConsumer", TestStringViewConsumer);

  run_test("NullTerminationForKnownTerminatedInput",
           TestNullTerminationForKnownTerminatedInput);
  run_test("MaterializesNonTerminatedView", TestMaterializesNonTerminatedView);
  run_test("MaterializesEmptyView", TestMaterializesEmptyView);

  run_test("ConcurrentCStr", TestConcurrentCStr);
  run_test("WideCharInstantiation", TestWideCharInstantiation);

  run_test("EmptyString", TestEmptyString);
  run_test("EmptyStringView", TestEmptyStringView);
  run_test("EmptyLiteral", TestEmptyLiteral);

  std::cout << "=== ALL TESTS PASSED ===\n";
}