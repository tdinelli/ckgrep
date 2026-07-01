# Search/parsing logic, with no CLI or main() attached, so it can be linked
# into both the ckgrep executable and the test binary.
add_library(
  ckgrep_core
  STATIC
  pattern.cpp
  query.cpp
  matcher.cpp
  reaction_parser.cpp
  reaction_scanner.cpp
  utils.cpp
)
target_include_directories(ckgrep_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ckgrep_core PUBLIC ckgrep_compiler_options)

# -----------------------------------------------------------------------------
# Main executable
# -----------------------------------------------------------------------------
add_executable(ckgrep cli.cpp main.cpp)
target_link_libraries(
  ckgrep
  PRIVATE ckgrep_core argparse ckgrep_compiler_options
)
target_compile_definitions(ckgrep PRIVATE CKGREP_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS ckgrep RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
