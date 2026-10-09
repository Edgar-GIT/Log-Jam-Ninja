// ContractHandler.cpp - what happens when a contract (a "pre" condition) is violated

#if defined(__cpp_contracts)

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <print>

//called by the compiler-generated code whenever a contract check fails
//its name and signature are fixed by the standard so it lives in the global namespace
void handle_contract_violation(const std::contracts::contract_violation& violation) {
    const auto where = violation.location();
    std::println(stderr, "Contract violated at {}:{} in {}\n  condition: {}",
                 where.file_name(), where.line(), where.function_name(), violation.comment());
    std::abort();
}

#endif  // defined(__cpp_contracts)
