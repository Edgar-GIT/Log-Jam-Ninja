cmake_minimum_required(VERSION 3.30)
project(LogJamNinja VERSION 0.1.0 LANGUAGES C CXX)

# ------------------------------------------------------------------ Language
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ------------------------------------------------------------------- Options
option(LJN_BUILD_GAME   "Build the game"                                  ON)
option(LJN_BUILD_EDITOR "Build the map editor"                            ON)
option(LJN_BUILD_TESTS  "Build the tests"                                 OFF)
option(LJN_NATIVE_ARCH  "Tune Release builds for this CPU (-march=native)" ON)

# --------------------------------------------------------- LTO (Release only)
include(CheckIPOSupported)
check_ipo_supported(RESULT LJN_IPO_SUPPORTED OUTPUT LJN_IPO_OUTPUT)
if(LJN_IPO_SUPPORTED)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

# -------------------------------------------------------------- Dependencies
include(FetchContent)

# Use the system package if it is installed, otherwise download it.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW ON
  FIND_PACKAGE_ARGS 5.5)

FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  FIND_PACKAGE_ARGS 3.11)

# plf::hive: reference implementation of std::hive (header-only, no CMake project).
# TODO: pin GIT_TAG to a commit once we settle on a version.
FetchContent_Declare(plf_hive
  GIT_REPOSITORY https://github.com/mattreecebentley/plf_hive.git
  GIT_TAG main
  GIT_SHALLOW ON)

FetchContent_MakeAvailable(raylib nlohmann_json plf_hive)

add_library(plf_hive INTERFACE)
target_include_directories(plf_hive SYSTEM INTERFACE ${plf_hive_SOURCE_DIR})

if(LJN_BUILD_EDITOR)
  # raygui is header-only: RAYGUI_IMPLEMENTATION must be defined in exactly one .cpp
  FetchContent_Declare(raygui
    GIT_REPOSITORY https://github.com/raysan5/raygui.git
    GIT_TAG 4.0
    GIT_SHALLOW ON)

  # Native file dialog (uses GTK3 on Linux)
  set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG v1.2.1
    GIT_SHALLOW ON)

  FetchContent_MakeAvailable(raygui nfd)

  add_library(raygui INTERFACE)
  target_include_directories(raygui SYSTEM INTERFACE ${raygui_SOURCE_DIR}/src)
  target_link_libraries(raygui INTERFACE raylib)
endif()

# ---------------------------------------------------------- Shared interfaces
# Warnings
add_library(ljn_warnings INTERFACE)
if(MSVC)
  target_compile_options(ljn_warnings INTERFACE /W4)
else()
  target_compile_options(ljn_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

# C++26 features and CPU tuning, inherited by everything that links ljn_core
add_library(ljn_cxx INTERFACE)
target_compile_features(ljn_cxx INTERFACE cxx_std_26)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include(CheckCXXCompilerFlag)
  set(CMAKE_REQUIRED_FLAGS "-std=c++2c")
  check_cxx_compiler_flag(-fcontracts  LJN_HAS_CONTRACTS)
  check_cxx_compiler_flag(-freflection LJN_HAS_REFLECTION)
  unset(CMAKE_REQUIRED_FLAGS)

  if(LJN_HAS_CONTRACTS)
    target_compile_options(ljn_cxx INTERFACE -fcontracts)
  endif()
  if(LJN_HAS_REFLECTION)
    target_compile_options(ljn_cxx INTERFACE -freflection)
  endif()
endif()

if(LJN_NATIVE_ARCH AND NOT MSVC)
  # Release already gets -O3 from CMake; this adds CPU-specific instructions.
  # Turn it off (-DLJN_NATIVE_ARCH=OFF) if you ever distribute the binaries.
  target_compile_options(ljn_cxx INTERFACE $<$<CONFIG:Release>:-march=native>)
endif()

# ------------------------------------------------------------------ ljn_core
# Shared by the game and the editor. Pure logic: it never links raylib.
file(GLOB_RECURSE LJN_CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp)
add_library(ljn_core STATIC ${LJN_CORE_SOURCES})
target_include_directories(ljn_core PUBLIC ${CMAKE_SOURCE_DIR}/src/core)
target_link_libraries(ljn_core
  PUBLIC  nlohmann_json::nlohmann_json plf_hive ljn_cxx
  PRIVATE ljn_warnings)

set(LJN_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

# ---------------------------------------------------------------------- Game
if(LJN_BUILD_GAME)
  file(GLOB_RECURSE LJN_GAME_SOURCES CONFIGURE_DEPENDS src/game/*.cpp)
  add_executable(log-jam-ninja ${LJN_GAME_SOURCES})
  target_include_directories(log-jam-ninja PRIVATE ${CMAKE_SOURCE_DIR}/src/game)
  target_link_libraries(log-jam-ninja PRIVATE ljn_core raylib ljn_warnings)
  target_compile_definitions(log-jam-ninja PRIVATE LJN_ASSETS_DIR="${LJN_ASSETS_DIR}")
endif()

# -------------------------------------------------------------------- Editor
if(LJN_BUILD_EDITOR)
  file(GLOB_RECURSE LJN_EDITOR_SOURCES CONFIGURE_DEPENDS src/editor/*.cpp)
  add_executable(ljn-editor ${LJN_EDITOR_SOURCES})
  target_include_directories(ljn-editor PRIVATE ${CMAKE_SOURCE_DIR}/src/editor)
  target_link_libraries(ljn-editor PRIVATE ljn_core raylib raygui nfd ljn_warnings)
  target_compile_definitions(ljn-editor PRIVATE LJN_ASSETS_DIR="${LJN_ASSETS_DIR}")
endif()

# --------------------------------------------------------------------- Tests
if(LJN_BUILD_TESTS AND EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
  enable_testing()
  add_subdirectory(tests)
endif()
