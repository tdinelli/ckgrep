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

#include <memory>

#include <argparse/argparse.hpp>

namespace ckgrep::cli {

/**
 * @brief Builds the ckgrep argument parser and parses argv into it.
 *
 * @details Defines every ckgrep flag and positional (`-h/--help`,
 * `-v/--version`, `query`, `files`, `-e/--exact`, `-c/--comments`), then
 * parses @p argc / @p argv against them.
 *
 * `-h/--help` and `-v/--version` are handled here directly, scanning argv
 * before argparse's own parsing runs: `query` is a required positional, so
 * letting argparse parse first would make `ckgrep --help` fail with a
 * missing-argument error instead of showing help. Both flags print and exit
 * the process immediately; this function does not return in that case.
 *
 * On a parse error (e.g. a missing required argument), prints the error
 * alongside usage and exits the process with status 1.
 *
 * @param argc Argument count, as passed to main().
 * @param argv Argument vector, as passed to main().
 * @return The parsed parser, ready for `program->get<...>(...)` calls.
 *
 * @note Returned via unique_ptr because argparse::ArgumentParser deletes its
 *       move constructor and so cannot be returned by value.
 */
std::unique_ptr<argparse::ArgumentParser> parse_args(int argc, char** argv);

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
