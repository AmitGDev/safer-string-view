SaferStringView is a single C++23 header file (`SaferStringView.hpp`) providing a safer
drop-in alternative to `std::string_view`. It stores rvalue strings by value (preventing dangling
references to temporaries) while still viewing lvalue strings and string literals without copying,
and can guarantee a null-terminated pointer on demand. `main.cpp` is the self-contained
demonstration/validation program and the authoritative reference for current behavior; there is
no separate binary to link against, only the header.