// check.hpp - tiny helpers shared by all test files.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <print>

//stops the program with a message if the condition is false
#define CHECK(...)                                                                    \
    do {                                                                              \
        if (!(__VA_ARGS__)) {                                                         \
            std::println(stderr, "FAIL {}:{}  {}", __FILE__, __LINE__, #__VA_ARGS__); \
            std::exit(1);                                                             \
        }                                                                             \
    } while (false)

//true if the result is an error and it is exactly the expected one
inline bool failsWith(const auto& result, auto error) {
    return !result.has_value() && result.error() == error;
}
