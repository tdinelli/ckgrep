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
#include <filesystem>
#include <iostream>
#include <vector>

#include "cli.hpp"
#include "matcher.hpp"
#include "query.hpp"
#include "string_utils.hpp"
#include "utils.hpp"

int main(int argc, char** argv) {
  // command line arguments parsing see cli.hpp/.cpp
  auto program = ckgrep::cli::parse_args(argc, argv);

  // matching mode
  ckgrep::match_mode mode = ckgrep::match_mode::contains;
  if (program->get<bool>("--exact")) {
    mode = ckgrep::match_mode::exact;
  }

  // parsing and analyzing the query this instantiate the query parsing engine see the
  // doc to understand how to generate an appropriate query
  ckgrep::query q;
  auto query_text = program->get<std::string>("query");
  try {
    q = ckgrep::parse_query(query_text);
  } catch (const std::exception& e) {
    std::cerr << "query error: " << e.what() << "\n";
    return 2;
  }

  auto files_arg = program->get<std::vector<std::string>>("files");
  if (files_arg.empty()) {
    files_arg.emplace_back(".");
  }

  std::vector<std::filesystem::path> files;
  for (const auto& file_path : files_arg) {
    auto resolved = ckgrep::utils::collect_files(file_path);
    files.insert(files.end(), resolved.begin(), resolved.end());
  }

  bool search_comments = program->get<bool>("--comments");

  bool show_path = files.size() > 1;
  int hits = 0;
  auto print_hit = [&](const std::filesystem::path& file,
                       std::size_t line_number,
                       const std::string& raw) {
    if (show_path) {
      std::cout << file.string() << ":";
    }
    std::cout << line_number << ": " << raw << "\n";
    ++hits;
  };

  for (const auto& file : files) {
    std::string file_content = ckgrep::utils::read_file(file);
    for (const ckgrep::reaction& r : ckgrep::scan_reactions(file_content)) {
      if (ckgrep::matches(q, r, mode)) {
        print_hit(file, r.line_number, r.raw);
      }
    }

    if (search_comments) {
      for (const ckgrep::comment& c : ckgrep::scan_comments(file_content)) {
        if (ckgrep::utils::contains_ci(c.text, query_text)) {
          print_hit(file, c.line_number, c.raw);
        }
      }
    }
  }

  return hits > 0 ? 0 : 1;
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
