`src\main.cpp` should demonstrate:

1. Construction from an lvalue `std::string` stores a view, not a copy.
2. Construction from an rvalue `std::string` (including temporaries returned by value, e.g. `std::to\_string`) takes ownership.
3. Construction from `std::string\_view` and from string literals / `const char\*`.
4. Copy construction and copy assignment preserve the expected ownership state of the source.
5. Move construction and move assignment preserve the expected ownership state of the source.
6. Implicit conversion to `std::string\_view` works as a drop-in replacement when passed to functions expecting `std::string\_view`.
7. Edge cases: empty strings, strings with special/escaped characters, and multi-byte Unicode content.

Keep the demonstration deterministic and avoid timing-dependent or environment-dependent behavior.



