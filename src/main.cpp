#define TEST_SAFERSTRINGVIEW  // Exposes private members for testing

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "SaferStringView.hpp"

// Helper function to test drop-in replacement
static void ConsumeStringView(std::string_view view) {
  std::cout << "Function received: \"" << view
            << "\" (length: " << view.length() << ")\n";
}

// Helper to check if SaferStringView owns the data
template <typename T>
static auto OwnsData(const SaferStringView<T>& ssv) {
  return std::holds_alternative<std::basic_string<T>>(ssv.storage_);
}

// NOLINTNEXTLINE(bugprone-exception-escape)
int main() {
  std::cout << "=== COMPREHENSIVE SaferStringView TEST SUITE ===\n\n";

  // Test 1: Construction from lvalue string (should store string_view)
  std::cout << "1. Construction from lvalue string:\n";
  const std::string str1 = "Hello World";
  const SaferStringView ssv1(str1);
  std::cout << "   Content: " << std::string_view(ssv1) << "\n";
  std::cout << "   Owns data: " << std::boolalpha << OwnsData(ssv1) << "\n\n";

  // Test 2: Construction from rvalue string (should take ownership)
  std::cout << "2. Construction from rvalue string:\n";
  const SaferStringView ssv2(std::string("Temporary String"));
  std::cout << "   Content: " << std::string_view(ssv2) << "\n";
  std::cout << "   Owns data: " << std::boolalpha << OwnsData(ssv2) << "\n\n";

  // Test 3: Construction from rvalue via function call
  std::cout << "3. Construction from rvalue function result:\n";
  constexpr int kTestNumber = 42;
  const SaferStringView ssv3(std::to_string(kTestNumber));
  std::cout << "   Content: " << std::string_view(ssv3) << "\n";
  std::cout << "   Owns data: " << std::boolalpha << OwnsData(ssv3) << "\n\n";

  // Test 4: Construction from string_view literal
  std::cout << "4. Construction from string_view literal:\n";
  const SaferStringView ssv4(std::string_view("String View Literal"));
  std::cout << "   Content: " << std::string_view(ssv4) << "\n";
  std::cout << "   Owns data: " << std::boolalpha << OwnsData(ssv4) << "\n\n";

  // Test 5: Construction from string_view of existing string
  std::cout << "5. Construction from string_view of existing string:\n";
  const std::string base_str = "Base String";
  const std::string_view sv_from_str(base_str);
  const SaferStringView ssv5(sv_from_str);
  std::cout << "   Content: " << std::string_view(ssv5) << "\n";
  std::cout << "   Owns data: " << std::boolalpha << OwnsData(ssv5) << "\n\n";

  // NEW Test 6: Construction from string literal (const char*)
  std::cout << "6. Construction from string literal (const char*):\n";
  const SaferStringView ssv6_a("String Literal");
  std::cout << "   Direct literal: \"" << std::string_view(ssv6_a)
            << "\" (owns: " << OwnsData(ssv6_a) << ")\n";

  const char* c_str = "C-style string";
  const SaferStringView ssv6_b(c_str);
  std::cout << "   From const char*: \"" << std::string_view(ssv6_b)
            << "\" (owns: " << OwnsData(ssv6_b) << ")\n";

  char buffer[] = "Mutable buffer";
  const SaferStringView ssv6_c(static_cast<const char*>(buffer));
  std::cout << "   From char array: \"" << std::string_view(ssv6_c)
            << "\" (owns: " << OwnsData(ssv6_c) << ")\n\n";

  // NEW Test 7: String literal edge cases
  std::cout << "7. String literal edge cases:\n";
  const SaferStringView ssv7_empty("");
  std::cout << "   Empty literal: \"" << std::string_view(ssv7_empty)
            << "\" (length: " << std::string_view(ssv7_empty).length()
            << ", owns: " << OwnsData(ssv7_empty) << ")\n";

  const SaferStringView ssv7_special("Special\nChars\t\"Quoted\"");
  std::cout << "   Special chars: \"" << std::string_view(ssv7_special)
            << "\" (owns: " << OwnsData(ssv7_special) << ")\n";

  const SaferStringView ssv7_unicode("Hello 世界 🌍");
  std::cout << "   Unicode: \"" << std::string_view(ssv7_unicode)
            << "\" (owns: " << OwnsData(ssv7_unicode) << ")\n\n";

  // Test 8: Copy construction
  std::cout << "8. Copy construction:\n";
  const SaferStringView<char>& ssv8_from_owned = ssv2;  // Copy from owned
  const SaferStringView<char>& ssv8_from_view = ssv1;   // Copy from view
  const SaferStringView<char>& ssv8_from_literal =
      ssv6_a;  // Copy from literal-based
  std::cout << "   Copy from owned: " << std::string_view(ssv8_from_owned)
            << " (owns: " << OwnsData(ssv8_from_owned) << ")\n";
  std::cout << "   Copy from view: " << std::string_view(ssv8_from_view)
            << " (owns: " << OwnsData(ssv8_from_view) << ")\n";
  std::cout << "   Copy from literal: " << std::string_view(ssv8_from_literal)
            << " (owns: " << OwnsData(ssv8_from_literal) << ")\n\n";

  // Test 9: Move construction
  std::cout << "9. Move construction:\n";
  SaferStringView temp_owned(std::string("Will be moved"));
  SaferStringView temp_view(str1);
  SaferStringView temp_literal("Literal to move");

  const SaferStringView ssv9_from_owned(std::move(temp_owned));
  const SaferStringView ssv9_from_view(std::move(temp_view));
  const SaferStringView ssv9_from_literal(std::move(temp_literal));

  std::cout << "   Moved from owned: " << std::string_view(ssv9_from_owned)
            << " (owns: " << OwnsData(ssv9_from_owned) << ")\n";
  std::cout << "   Moved from view: " << std::string_view(ssv9_from_view)
            << " (owns: " << OwnsData(ssv9_from_view) << ")\n";
  std::cout << "   Moved from literal: " << std::string_view(ssv9_from_literal)
            << " (owns: " << OwnsData(ssv9_from_literal) << ")\n";
  std::cout << "   (Original values after move are in unspecified state)\n\n";

  // Test 10: Copy assignment
  std::cout << "10. Copy assignment:\n";
  SaferStringView ssv10(std::string("Initial"));
  std::cout << "   Before assignment: " << std::string_view(ssv10)
            << " (owns: " << OwnsData(ssv10) << ")\n";

  ssv10 = ssv2;  // Assign from owned
  std::cout << "   After assign from owned: " << std::string_view(ssv10)
            << " (owns: " << OwnsData(ssv10) << ")\n";

  ssv10 = ssv1;  // Assign from view
  std::cout << "   After assign from view: " << std::string_view(ssv10)
            << " (owns: " << OwnsData(ssv10) << ")\n";

  ssv10 = ssv6_a;  // Assign from literal
  std::cout << "   After assign from literal: " << std::string_view(ssv10)
            << " (owns: " << OwnsData(ssv10) << ")\n\n";

  // Test 11: Move assignment
  std::cout << "11. Move assignment:\n";
  SaferStringView ssv11(std::string("Initial"));
  SaferStringView temp_for_move1(std::string("Move Source 1"));
  SaferStringView temp_for_move2(str1);
  SaferStringView temp_for_move3("Literal Move Source");

  ssv11 = std::move(temp_for_move1);  // Move assign from owned
  std::cout << "   After move assign from owned: " << std::string_view(ssv11)
            << " (owns: " << OwnsData(ssv11) << ")\n";

  ssv11 = std::move(temp_for_move2);  // Move assign from view
  std::cout << "   After move assign from view: " << std::string_view(ssv11)
            << " (owns: " << OwnsData(ssv11) << ")\n";

  ssv11 = std::move(temp_for_move3);  // Move assign from literal
  std::cout << "   After move assign from literal: " << std::string_view(ssv11)
            << " (owns: " << OwnsData(ssv11) << ")\n\n";

  // Test 12: Drop-in replacement functionality
  std::cout << "12. Drop-in string_view replacement:\n";
  ConsumeStringView(ssv1);    // From lvalue string
  ConsumeStringView(ssv2);    // From rvalue string (owned)
  ConsumeStringView(ssv4);    // From string_view
  ConsumeStringView(ssv6_a);  // From string literal
  ConsumeStringView(SaferStringView(std::string("Direct temp")));  // Temporary
  ConsumeStringView(
      SaferStringView("Direct literal temp"));  // Literal temporary
  std::cout << "\n";

  // Test 13: Mixed constructor usage in function calls
  std::cout << "13. Mixed constructor usage:\n";
  const auto test_function = [](const SaferStringView<char>& view) {
    std::cout << "   Received: \"" << std::string_view(view)
              << "\" (owns: " << OwnsData(view) << ")\n";
  };

  const std::string owned_str = "Owned string";
  test_function(SaferStringView(owned_str));                   // lvalue string
  test_function(SaferStringView(std::string("Temp string")));  // rvalue string
  test_function(SaferStringView(std::string_view("sv")));      // string_view
  test_function(SaferStringView("literal"));                   // const char*
  std::cout << "\n";

  // Test 14: Chained operations
  std::cout << "14. Chained operations:\n";
  const SaferStringView chain1(std::string("Chain Start"));
  SaferStringView chain2 = chain1;
  SaferStringView chain3 = std::move(chain2);
  constexpr int kChainNumber = 999;
  chain3 = SaferStringView(std::to_string(kChainNumber));
  chain3 = SaferStringView("Final literal value");
  std::cout << "   Final chain result: " << std::string_view(chain3)
            << " (owns: " << OwnsData(chain3) << ")\n\n";

  // Test 15: All empty variants
  std::cout << "15. Edge cases - all empty variants:\n";
  const SaferStringView empty_str(std::string(""));
  const SaferStringView empty_sv(std::string_view(""));
  const SaferStringView empty_literal("");
  std::cout << "   Empty string: \"" << std::string_view(empty_str)
            << "\" (length: " << std::string_view(empty_str).length()
            << ", owns: " << OwnsData(empty_str) << ")\n";
  std::cout << "   Empty string_view: \"" << std::string_view(empty_sv)
            << "\" (length: " << std::string_view(empty_sv).length()
            << ", owns: " << OwnsData(empty_sv) << ")\n";
  std::cout << "   Empty literal: \"" << std::string_view(empty_literal)
            << "\" (length: " << std::string_view(empty_literal).length()
            << ", owns: " << OwnsData(empty_literal) << ")\n\n";

  // Test 16: Comparison of all construction methods
  std::cout << "16. Summary - Ownership by construction method:\n";
  std::cout << "   From lvalue string:     "
            << (OwnsData(ssv1) ? "OWNS" : "VIEWS") << "\n";
  std::cout << "   From rvalue string:     "
            << (OwnsData(ssv2) ? "OWNS" : "VIEWS") << "\n";
  std::cout << "   From function result:   "
            << (OwnsData(ssv3) ? "OWNS" : "VIEWS") << "\n";
  std::cout << "   From string_view:       "
            << (OwnsData(ssv4) ? "OWNS" : "VIEWS") << "\n";
  std::cout << "   From string literal:    "
            << (OwnsData(ssv6_a) ? "OWNS" : "VIEWS") << "\n";
  std::cout << "   From const char*:       "
            << (OwnsData(ssv6_b) ? "OWNS" : "VIEWS") << "\n\n";

  std::cout << "=== ALL TESTS COMPLETED ===\n";
}