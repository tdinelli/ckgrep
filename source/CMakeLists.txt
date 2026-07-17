# -----------------------------------------------------------------------------
# ckgrep_core - search/parsing logic, with no CLI or main() attached, so it
# can be linked into both the ckgrep executable and the test binary. The
# compiler flags arrive through ckgrep_compiler_options and, being linked
# PUBLIC, propagate to every target that links the core library.
# -----------------------------------------------------------------------------
add_library(
  ckgrep_core
  STATIC
  pattern.cpp
  query.cpp
  matcher.cpp
  reaction_parser.cpp
  reaction_scanner.cpp
  species_scanner.cpp
  utils.cpp
)
target_include_directories(ckgrep_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ckgrep_core PUBLIC ckgrep_compiler_options)
# Single source of truth for the version printed by --version and the logo:
# the project() version in the top-level CMakeLists.txt. Defined PUBLIC on the
# core library because both utils.cpp (core) and cli.cpp (executable) use it.
target_compile_definitions(ckgrep_core PUBLIC CKGREP_VERSION="${PROJECT_VERSION}")

# -----------------------------------------------------------------------------
# Main executable
# -----------------------------------------------------------------------------
add_executable(ckgrep cli.cpp main.cpp)
target_link_libraries(ckgrep PRIVATE ckgrep_core argparse)

# -----------------------------------------------------------------------------
# Installation - `cmake --install <build dir> [--prefix <path>]`
# The PDF manual, when built, is installed by cmake/Documentation.cmake.
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS ckgrep RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
