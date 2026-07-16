/* ----------------------------------------------------------------------------------- *\
|                                 __                                                    |
|                           _____/ /______ _________  ____                              |
|                          / ___/ //_/ __ `/ ___/ _ \/ __ \                             |
|                         / /__/ ,< / /_/ / /  /  __/ /_/ /                             |
|                         \___/_/|_|\__, /_/   \___/ .___/                              |
|                                  /____/         /_/                                   |
|                                                                                       |
| ------------------------------------------------------------------------------------- |
|  See license and copyright at the end of this file.                                   |
| ------------------------------------------------------------------------------------- |
|                                                                                       |
|          Author: Timoteo Dinelli  <timoteo.dinelli@polimi.it>                         |
|                                                                                       |
|                  CRECK Modeling Lab <https://www.creckmodeling.polimi.it>             |
|                  Department of Chemistry, Materials and Chemical Engineering          |
|                  Politecnico di Milano, P.zza Leonardo da Vinci 32, 20133 Milano      |
|                                                                                       |
\* ----------------------------------------------------------------------------------- */
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "cli.hpp"
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

  program->add_description(
      "Searches CHEMKIN reaction mechanisms by chemistry, not by text: queries "
      "name species,\nnot strings, so CH4 does not match CH4O and H+H matches 2H."
  );

  // clang-format off
  program->add_epilog(
      "Query syntax:\n"
      "  A query is one or more species, optionally split across a reaction arrow\n"
      "  (=, =>, <=>) to anchor reactants and/or products. Species on one side are\n"
      "  joined with + and are all required. Matching is case-insensitive.\n"
      "\n"
      "    CH4          reactions where CH4 participates, on either side\n"
      "    OH+CH4=      OH and CH4 together as reactants\n"
      "    =CO2         CO2 among the products\n"
      "    =2H          two H atoms produced, spelled 2H or H+H in the file\n"
      "    C3H5*        any species starting with C3H5 (globs match whole names)\n"
      "    ?C3H7        NC3H7 or IC3H7 (? matches exactly one character)\n"
      "    (+M)         fall-off reactions; a bare M: third-body reactions\n"
      "\n"
      "Examples:\n"
      "  ckgrep \"CH4\" mechanism.CKI             methane chemistry in one file\n"
      "  ckgrep -e \"H+O2=OH+O\" kinetics/        exact reaction, whole directory\n"
      "  ckgrep -c \"CH3O2=CH2O+OH\" mech.CKI     commented-out reactions too\n"
      "  ckgrep -p \"CH4\" mechanism.CKI          normalized, column-aligned output\n"
      "\n"
      "Always quote the query: =, *, ?, ( and ) are shell metacharacters.\n"
      "Exits 0 when something matched, 1 when nothing did, 2 on a malformed query."
  );

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
  program->add_argument("-i", "--input")
      .help("Explicit alias for the kinetics 'files' positional")
      .nargs(argparse::nargs_pattern::at_least_one)
      .default_value(std::vector<std::string>{});
  program->add_argument("--thermo")
      .help("External thermo file(s) to search alongside the kinetics input")
      .nargs(argparse::nargs_pattern::at_least_one)
      .default_value(std::vector<std::string>{});
  program->add_argument("--transport")
      .help("External transport file(s) to search alongside the kinetics input")
      .nargs(argparse::nargs_pattern::at_least_one)
      .default_value(std::vector<std::string>{});
  program->add_argument("-s", "--species")
      .help("Search for the species name instead of matching reactions name.")
      .flag();
  program->add_argument("-e", "--exact")
      .help("Whether the query must match exactly")
      .flag();
  program->add_argument("-c", "--comments")
      .help("Also match commented-out reactions (text after '!' that parses as a "
            "matching reaction)")
      .flag();
  program->add_argument("-p", "--pretty")
      .help("Reformat matches from the parsed reaction: normalized spacing, "
            "aligned rate columns, comments dropped")
      .flag();
  program->add_argument("-H", "--with-filename")
      .help("Always print the file name, even when searching a single file "
            "(for editor/tooling integration)")
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
      std::cout << " Version: " << CKGREP_VERSION << "\n";
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
/* ----------------------------------------------------------------------------------- *\
|                                                                                       |
|     GNU General Public License v3 (GPL-3)                                             |
|                                                                                       |
|     Copyright (c) 2026 Timoteo Dinelli                                                |
|                                                                                       |
|     This program is free software: you can redistribute it and/or modify it           |
|     under the terms of the GNU General Public License as published by the Free        |
|     Software Foundation, either version 3 of the License, or (at your option)         |
|     any later version.                                                                |
|                                                                                       |
|     This program is distributed in the hope that it will be useful, but WITHOUT       |
|     ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS     |
|     FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.    |
|                                                                                       |
|     You should have received a copy of the GNU General Public License along with      |
|     this program. If not, see <https://www.gnu.org/licenses/>.                        |
|                                                                                       |
\* ----------------------------------------------------------------------------------- */
