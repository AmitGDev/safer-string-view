`src\main.cpp` should demonstrate:

1. Construction from an lvalue `std::string` stores a view, not a copy.
2. Construction from an rvalue `std::string` (including temporaries returned by value, e.g. `std::to_string`) takes ownership.
3. Construction from `std::string_view` and from string literals / `const char*`.
4. Copy construction and copy assignment preserve the expected ownership state of the source.
5. Move construction and move assignment preserve the expected ownership state of the source.
6. Implicit conversion to `std::string_view` works as a drop-in replacement when passed to functions expecting `std::string_view`.
7. Edge cases: empty strings, strings with special/escaped characters, and multi-byte Unicode content.
8. `c_str()` / `null_terminated()` behavior, including lazy materialization, concurrent calls, and non-`char` instantiations.
9. Read `src\main.cpp` directly for the authoritative, current list of covered cases and exact wording; do not rely on a summary, since tests may be added, renamed, or restructured.

Keep the demonstration deterministic and avoid timing-dependent or environment-dependent behavior. Concurrency tests may vary in interleaving, but the asserted outcome must remain deterministic and always pass.