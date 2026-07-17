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
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "cli.hpp"
#include "query.hpp"
#include "reaction_scanner.hpp"
#include "species_scanner.hpp"
#include "utils.hpp"

int main(int argc, char** argv) {
  // command line arguments parsing see cli.hpp/.cpp
  auto program = ckgrep::cli::parse_args(argc, argv);

  // parsing and analyzing the query this instantiate the query parsing engine see the
  // doc to understand how to generate an appropriate query
  auto query_text = program->get<std::string>("query");
  bool species_mode = program->get<bool>("--species");
  ckgrep::query q;
  try {
    q = ckgrep::parse_query(query_text, /*case_sensitive=*/false, species_mode);
  } catch (const std::exception& e) {
    std::cerr << "query error: " << e.what() << "\n";
    return 2;
  }

  ckgrep::search_options opts;
  if (program->get<bool>("--exact")) {
    opts.mode = ckgrep::match_mode::exact;
  }
  opts.search_comments = program->get<bool>("--comments");
  opts.pretty = program->get<bool>("--pretty");

  auto input_arg = program->get<std::vector<std::string>>("--input");
  auto thermo_arg = program->get<std::vector<std::string>>("--thermo");
  auto transport_arg = program->get<std::vector<std::string>>("--transport");
  auto files_arg = program->get<std::vector<std::string>>("files");
  files_arg.insert(files_arg.end(), input_arg.begin(), input_arg.end());
  if (files_arg.empty() && thermo_arg.empty() && transport_arg.empty()) {
    files_arg.emplace_back(std::filesystem::current_path().string());
  }

  std::vector<std::filesystem::path> files = ckgrep::utils::collect_files(
      std::vector<std::filesystem::path>(files_arg.begin(), files_arg.end())
  );
  std::vector<std::filesystem::path> thermo_files = ckgrep::utils::collect_files(
      std::vector<std::filesystem::path>(thermo_arg.begin(), thermo_arg.end())
  );
  std::vector<std::filesystem::path> transport_files = ckgrep::utils::collect_files(
      std::vector<std::filesystem::path>(transport_arg.begin(), transport_arg.end())
  );

  std::size_t total_sources =
      files.size() + thermo_files.size() + transport_files.size();
  bool show_path = total_sources > 1 || program->get<bool>("--with-filename");
  std::size_t total_hits = 0;

  if (species_mode) {
    // parse_query() with species_only=true always populates q.any with
    // exactly one token, or throws, caught above, so this should be safe.
    if (!q.any.has_value()) {
      std::cerr << "ckgrep: internal error: species query produced no pattern\n";
      return 2;
    }
    const ckgrep::species_pattern& pattern = *q.any->tokens.front();

    for (const auto& file : files) {
      std::string file_content;
      try {
        file_content = ckgrep::utils::read_file(file);
      } catch (const std::exception& e) {
        std::cerr << "ckgrep: " << e.what() << "\n";
        continue;
      }
      for (const ckgrep::search_hit& hit :
           ckgrep::scan_species_block(file_content, pattern, opts)) {
        ckgrep::utils::print_results(file, hit.line_number, hit.text, show_path);
        ++total_hits;
      }
    }

    for (const auto& file : thermo_files) {
      std::string file_content;
      try {
        file_content = ckgrep::utils::read_file(file);
      } catch (const std::exception& e) {
        std::cerr << "ckgrep: " << e.what() << "\n";
        continue;
      }
      for (const ckgrep::search_hit& hit :
           ckgrep::scan_thermo_block(file_content, pattern, opts)) {
        ckgrep::utils::print_results(file, hit.line_number, hit.text, show_path);
        ++total_hits;
      }
    }

    for (const auto& file : transport_files) {
      std::string file_content;
      try {
        file_content = ckgrep::utils::read_file(file);
      } catch (const std::exception& e) {
        std::cerr << "ckgrep: " << e.what() << "\n";
        continue;
      }
      for (const ckgrep::search_hit& hit :
           ckgrep::scan_transport_lines(file_content, pattern, opts)) {
        ckgrep::utils::print_results(file, hit.line_number, hit.text, show_path);
        ++total_hits;
      }
    }
  } else {
    for (const auto& file : files) {
      std::string file_content;
      try {
        file_content = ckgrep::utils::read_file(file);
      } catch (const std::exception& e) {
        // One unreadable file must not abort the search of the others.
        std::cerr << "ckgrep: " << e.what() << "\n";
        continue;
      }

      for (const ckgrep::search_hit& hit : ckgrep::search_text(file_content, q, opts)) {
        ckgrep::utils::print_results(file, hit.line_number, hit.text, show_path);
        ++total_hits;
      }
    }
  }

  return total_hits > 0 ? 0 : 1;
}
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
