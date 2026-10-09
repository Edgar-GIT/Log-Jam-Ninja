#one small program per test file, all linked to the shared core
foreach(name map_test validator_test)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE ljn_core ljn_warnings)
  add_test(NAME ${name} COMMAND ${name})
endforeach()
