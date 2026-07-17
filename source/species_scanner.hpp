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

#include <string_view>
#include <vector>

#include "pattern.hpp"
#include "reaction_scanner.hpp"

namespace ckgrep {
/**
 * @brief Scans a SPECIES/SPEC ... END block and returns species matching @p pattern.
 *
 * @details Species names are whitespace-separated tokens on the lines between
 * a SPECIES/SPEC keyword line and the matching END, exactly as scanned for
 * reaction-keyword skipping in reaction_scanner. Each token is tested against
 * @p pattern independently, so a line with several species contributes one
 * hit per matching token.
 *
 * Comment handling mirrors search_text(): a line is truncated at its first
 * '!' and only the part before it is searched by default. When
 * search_options::search_comments is set, a comment body (after one or more
 * leading '!') is tokenized and searched the same way, so a commented-out
 * species declaration can still be found.
 *
 * @param text The full text of a kinetics file.
 * @param pattern The species pattern to test each token against.
 * @param opts Comment-search setting; see search_options.
 * @return Every matching species occurrence, in source order.
 */
std::vector<search_hit> scan_species_block(
    std::string_view text,
    const species_pattern& pattern,
    const search_options& opts
);

/**
 * @brief Scans a THERMO/THER ... END block and returns species matching @p pattern.
 *
 * @details Walks entries in the classic NASA 7-coefficient polynomial format:
 * a header line carries the species name (its first whitespace token) and
 * ends with the digit '1', optionally followed by '&' when the elemental
 * composition overflows onto its own continuation line instead of being
 * packed into the header; the coefficient lines that follow end with '2',
 * '3', and '4' respectively. Only header lines are inspected for a species
 * name -- composition-continuation and coefficient lines are skipped without
 * being validated, since their content is never needed here. A line that
 * does not fit any recognized shape is skipped rather than treated as an
 * error: this is a search, not a thermo-file validator.
 *
 * Comment handling mirrors scan_species_block()/search_text(): each line is
 * truncated at its first '!' by default, and -- when
 * search_options::search_comments is set -- a commented-out header line is
 * also recognized and searched.
 *
 * @param text The full text of a thermo file (or a THERMO block within a
 *             kinetics file).
 * @param pattern The species pattern to test each header's name against.
 * @param opts Comment-search setting; see search_options.
 * @return Every matching species, in source order.
 */
std::vector<search_hit> scan_thermo_block(
    std::string_view text,
    const species_pattern& pattern,
    const search_options& opts
);

/**
 * @brief Scans a transport file and returns species matching @p pattern.
 *
 * @details Transport files carry no SPECIES/END-style keyword wrapper: every
 * non-comment, non-blank line is a data line whose first whitespace token is
 * a species name, followed by numeric transport parameters and an optional
 * trailing '!'-comment.
 *
 * Comment handling mirrors scan_species_block()/search_text(): each line is
 * truncated at its first '!' by default, and -- when
 * search_options::search_comments is set -- a fully commented-out data line
 * is also recognized and searched.
 *
 * @param text The full text of a transport file.
 * @param pattern The species pattern to test each line's name against.
 * @param opts Comment-search setting; see search_options.
 * @return Every matching species, in source order.
 */
std::vector<search_hit> scan_transport_lines(
    std::string_view text,
    const species_pattern& pattern,
    const search_options& opts
);
}  // namespace ckgrep
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
