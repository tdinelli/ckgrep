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
#include <array>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "utils.hpp"

#ifndef CKGREP_VERSION
#define CKGREP_VERSION "unknown"
#endif

namespace ckgrep::utils {
void print_logo(std::ostream& os) {
  // clang-format off
  static constexpr std::array<std::string_view, 6> logo = {
    R"(           __                                                                            )",
    R"(     _____/ /______ _________  ____                                                      )",
    R"(    / ___/ //_/ __ `/ ___/ _ \/ __ \                                                     )",
    R"(   / /__/ ,< / /_/ / /  /  __/ /_/ /                                                     )",
    R"(   \___/_/|_|\__, /_/   \___/ .___/                                                      )",
    R"(            /____/         /_/                                                           )",
  };

  for (const auto& line : logo) {
    os << line << '\n';
  }

  static constexpr int field_width = 74;
  os << '\n';
  os << std::format("  Version   : {:<{}} \n", CKGREP_VERSION, field_width);
  os << std::format("  Build date: {:<{}} \n", __DATE__, field_width);
  os << "-----------------------------------------------------------------------------------------\n";
  // clang-format on
}

void print_license(std::ostream& os) {
  // clang-format off
  static constexpr std::array<std::string_view, 9> notice = {
    R"(-----------------------------------------------------------------------------------------)",
    R"(   ckgrep  Copyright (C) 2026  Timoteo Dinelli                                           )",
    R"(                                                                                         )",
    R"(   This program comes with ABSOLUTELY NO WARRANTY.                                       )",
    R"(   This is free software, and you are welcome to redistribute it                         )",
    R"(   under certain conditions.                                                             )",
    R"(                                                                                         )",
    R"(   For details see <https://www.gnu.org/licenses/gpl-3.0.html>.                          )",
    R"(-----------------------------------------------------------------------------------------)",
  };
  // clang-format on

  for (const auto& line : notice) {
    os << line << '\n';
  }
}

std::string read_file(const std::filesystem::path& path) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(path, ec) || ec) {
    throw std::runtime_error("not a readable file: " + path.string());
  }

  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("cannot open file: " + path.string());
  }

  const auto size = std::filesystem::file_size(path, ec);
  std::string content;
  if (!ec) {
    content.reserve(size);
  }

  content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return content;
}

std::vector<std::filesystem::path> collect_files(const std::filesystem::path& path) {
  std::vector<std::filesystem::path> files;

  std::error_code ec;
  if (std::filesystem::is_regular_file(path, ec)) {
    files.push_back(path);
    return files;
  }

  if (!std::filesystem::is_directory(path, ec)) {
    std::cerr << "ckgrep: not a readable file or directory: " << path.string() << "\n";
    return files;
  }

  try {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             path,
             std::filesystem::directory_options::skip_permission_denied
         )) {
      if (entry.is_regular_file()) {
        files.push_back(entry.path());
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "ckgrep: " << e.what() << "\n";
  }

  return files;
}

// ANSI colors only when stdout is an interactive terminal -- pipes and
// redirections must stay clean -- and the user has not opted out through the
// NO_COLOR convention (https://no-color.org). On Windows the console needs
// virtual-terminal processing switched on before it understands the codes.
bool stdout_supports_color() {
  if (std::getenv("NO_COLOR") != nullptr) {
    return false;
  }
#ifdef _WIN32
  if (_isatty(_fileno(stdout)) == 0) {
    return false;
  }
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (handle == INVALID_HANDLE_VALUE || GetConsoleMode(handle, &mode) == 0) {
    return false;
  }
  return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
  return isatty(fileno(stdout)) == 1;
#endif
}

void print_results(
    const std::filesystem::path& path,
    const std::size_t line_number,
    const std::string_view content,
    const bool show_path
) {
  // grep's default palette: magenta file name, green line number, red separators; the
  // matched text itself stays uncolored.
  static const bool color = stdout_supports_color();
  const std::string_view path_color = color ? "\033[35m" : "";
  const std::string_view line_color = color ? "\033[32m" : "";
  const std::string_view sep_color = color ? "\033[31m" : "";
  const std::string_view reset = color ? "\033[0m" : "";

  if (show_path) {
    std::cout << path_color << path.string() << reset << sep_color << ":" << reset;
  }
  std::cout << line_color << line_number << reset << sep_color << ":" << reset << " "
            << content << "\n";
}
}  // namespace ckgrep::utils
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
