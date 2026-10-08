# One test program that uses the shared core library.
add_executable(ljn_tests map_test.cpp)
target_link_libraries(ljn_tests PRIVATE ljn_core ljn_warnings)

# Register it so "ctest" can run it.
add_test(NAME ljn_tests COMMAND ljn_tests)
