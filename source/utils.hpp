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
#pragma once

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace ckgrep::utils {
/**
 * @brief Prints the ckgrep banner to @p os.
 *
 * @details Writes the ASCII-art logo followed by the build version
 * (CKGREP_VERSION) and build date. Used for both the `--help` screen and
 * error output, so the banner always appears with the version it came from.
 *
 * @param os Stream to write to. Defaults to std::cout.
 */
void print_logo(std::ostream& os = std::cout);

/**
 * @brief Prints the GPL-3 notice to @p os.
 *
 * @details A short copyright/warranty disclaimer, shown alongside the banner
 * on `--help` and on usage errors.
 *
 * @param os Stream to write to. Defaults to std::cout.
 */
void print_license(std::ostream& os = std::cout);

/**
 * @brief Reads a file fully into memory.
 *
 * @param path Path to the file to read.
 * @return The file's full contents.
 *
 * @throws std::runtime_error if @p path does not exist, is not a regular
 *         file, or cannot be opened.
 */
std::string read_file(const std::filesystem::path& path);

/**
 * @brief Resolves one CLI-supplied path into the regular files to search.
 *
 * @details Centralizes the file/directory decision so main() can treat every
 * positional `files` argument the same way, regardless of whether the user
 * passed a single file or a directory.
 *
 * - @p path is a regular file: returns `{path}`.
 * - @p path is a directory: every regular file found while recursively
 *   walking the tree; a filesystem error encountered mid-walk is reported to
 *   stderr rather than thrown.
 * - @p path is neither: reports an error to stderr and returns an empty
 *   list.
 *
 * @param path The CLI-supplied path to resolve.
 * @return The regular files to search; empty if @p path could not be
 *         resolved to any.
 */
std::vector<std::filesystem::path> collect_files(const std::filesystem::path& path);

/**
 * @brief Prints one matched line to stdout in grep style.
 *
 * @details Output format is `line: content`, prefixed with `path:` when
 * @p show_path is set (i.e. when more than one file is being searched).
 * Printing only -- hit counting is the caller's job.
 *
 * When stdout is an interactive terminal the file name and line number are
 * colorized with grep's default palette (magenta name, green number, cyan
 * separators); output to pipes and files stays plain, and setting the
 * NO_COLOR environment variable disables colors everywhere.
 *
 * @param path        The file the match came from; printed when @p show_path.
 * @param line_number 1-based line number of the match.
 * @param content     The matched line text.
 * @param show_path   Whether to prefix the output with @p path.
 */
void print_results(
    const std::filesystem::path& path,
    std::size_t line_number,
    std::string_view content,
    bool show_path
);
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
