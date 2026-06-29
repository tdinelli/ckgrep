#include "cli.hpp"

#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "utils.hpp"

#ifndef CKGREP_VERSION
#define CKGREP_VERSION "unknown"
#endif

namespace ckgrep::cli {

std::unique_ptr<argparse::ArgumentParser> parse_args(int argc, char** argv) {
  auto program = std::make_unique<argparse::ArgumentParser>(
      "ckgrep",
      CKGREP_VERSION,
      argparse::default_arguments::none
  );

  // clang-format off
  program->add_argument("-h", "--help")
      .help("Show this help message and exit")
      .flag();
  program->add_argument("-v", "--version")
      .help("Print version information and exit")
      .flag();
  program->add_argument("query")
      .help("Query to grep for in the file(s)");
  program->add_argument("files")
      .help(
          "File(s) or directory/directories to search the query in. Directories are\n"
          "searched recursively. Defaults to the current directory if omitted."
      )
      .nargs(argparse::nargs_pattern::any)
      .default_value(std::vector<std::string>{});
  program->add_argument("-e", "--exact")
      .help("Whether the query must match exactly")
      .flag();
  program->add_argument("-c", "--comments")
      .help("Also search comment text (lines/trailing text after '!') for the query")
      .flag();
  // clang-format on

  // Handle --help before full parsing, since 'query' is required and would
  // otherwise make `ckgrep --help` fail.
  for (int i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (arg == "-h" || arg == "--help") {
      ckgrep::utils::print_logo();
      std::cout << *program;
      ckgrep::utils::print_license();
      std::exit(0);
    }
    if (arg == "-v" || arg == "--version") {
      std::cout << std::format(" Version: {}\n", CKGREP_VERSION);
      std::exit(0);
    }
  }

  try {
    program->parse_args(argc, argv);
  } catch (const std::exception& err) {
    ckgrep::utils::print_logo(std::cerr);
    std::cerr << err.what() << "\n\n" << *program;
    ckgrep::utils::print_license(std::cerr);
    std::exit(1);
  }

  return program;
}
}  // namespace ckgrep::cli
