// Contracts.hpp - lets us write preconditions that only compile where contracts exist.
#pragma once

// With GCC (-fcontracts) LJN_PRE(x) becomes the real C++26 "pre(x)" contract.
// With a compiler that lacks contracts it becomes nothing, so the code still builds.
#if defined(__cpp_contracts)
#define LJN_PRE(...) pre(__VA_ARGS__)
#else
#define LJN_PRE(...)
#endif
