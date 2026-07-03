# =============================================================================
# ckgrep - a grep-like tool for searching CHEMKIN reaction mechanisms
#
# Build options:
#   BUILD_TESTING    Build the test suite (default ON, target: ckgrep_tests)
#   BUILD_DOC        Enable the Doxygen PDF manual (default OFF, target: docs)
#   ENABLE_COVERAGE  Clang source-based coverage (default OFF, target: coverage)
#
# Layout:
#   source/          core library + ckgrep executable
#   tests/           GoogleTest suite (and the coverage report target)
#   cmake/           helper modules; cmake/external/ fetches dependencies
#   docs/            Doxygen configuration and the user guide
# =============================================================================
cmake_minimum_required(VERSION 3.20)
project(
  ckgrep
  VERSION 1.0.0
  LANGUAGES CXX
  DESCRIPTION "A grep-like tool for searching CHEMKIN reaction mechanisms"
)

# Default to Release if no build type is set (single-config generators only)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()

# Global language settings. The C++20 requirement itself is attached to
# targets through ckgrep_compiler_options (see cmake/CompilerOptions.cmake).
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # never fall back to an older standard
set(CMAKE_CXX_EXTENSIONS OFF)         # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compile_commands.json for clangd & co.

# Compiler support check, warning flags, and optional coverage instrumentation
include(cmake/CompilerOptions.cmake)

# External dependencies are fetched at configure time; no system packages needed
include(cmake/external/argparse.cmake)

add_subdirectory(source)

# Opt-in 'docs' target building the Doxygen PDF manual (see docs/Doxyfile.in)
option(BUILD_DOC "Build the ckgrep documentation" OFF)
if(BUILD_DOC)
  include(cmake/Documentation.cmake)
endif()

# Tests are on by default; -DBUILD_TESTING=OFF also skips fetching googletest
option(BUILD_TESTING "Build the ckgrep test suite" ON)
if(BUILD_TESTING)
  include(CTest)
  include(cmake/external/googletest.cmake)
  add_subdirectory(tests)
endif()
