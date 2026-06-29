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
#include <optional>
#include <string_view>
#include <vector>

#include "pattern.hpp"

namespace ckgrep {
/**
 * @brief One constrained side of a query: a conjunction of species patterns.
 *
 * @details Holds the pattern tokens parsed from a single side of a query (the
 * reactants, the products, or the whole arrow-less query). The tokens are an
 * AND-set: every pattern in @ref tokens must match some species on the
 * corresponding side of a reaction for that side to be considered satisfied.
 *
 * Patterns are stored as owning polymorphic handles, so a side may freely mix
 * literal, glob, and (in future) regex tokens without knowing their concrete
 * types.
 */
struct query_side {
  std::vector<std::unique_ptr<species_pattern>>
      tokens;  ///< AND-set of patterns for this side.
};

/**
 * @brief A parsed query, ready to be tested against reactions.
 *
 * @details A query takes one of two shapes depending on whether the user wrote
 * a reaction arrow:
 *
 * - **Arrow present**: matching is anchored to sides. The side(s) the user
 *   constrained are populated -- @ref reactants for text left of the arrow,
 *   @ref products for text to its right -- and @ref any is left empty. A side
 *   the user left blank stays unset, meaning that side is unconstrained.
 *
 * - **No arrow**: only @ref any is populated, and it is tested against the
 *   reactants OR the products of a reaction (either-side default).
 *
 * Exactly one mode is active per query: either @ref any is set, or one/both of
 * @ref reactants and @ref products are set, never both modes at once.
 */
struct query {
  std::optional<query_side>
      reactants;  ///< Constraints left of the arrow; unset if absent.
  std::optional<query_side>
      products;                   ///< Constraints right of the arrow; unset if absent.
  std::optional<query_side> any;  ///< Arrow-less query, matched against either side.
};

/**
 * @brief Parses a query string into a @ref query.
 *
 * @details Recognizes the same reaction arrows as CHEMKIN mechanisms, tried
 * longest-form first so the shorter arrows do not match inside the longer ones:
 * `<=>`, then `=>`, then `=`. Text on each side of the arrow is split into
 * tokens on `+`, and each non-empty token is trimmed and handed to
 * make_pattern(). When no arrow is present the whole string is parsed as a
 * single either-side group (see @ref query::any).
 *
 * The @p case_sensitive and @p use_regex flags are forwarded unchanged to every
 * pattern built for the query, so a single query is uniformly case-sensitive (or
 * not) and uniformly regex (or not).
 *
 * @param text           The raw query string, e.g. "CH4 + O2 => *OH".
 * @param case_sensitive When true, patterns require exact-case matching.
 *                       Defaults to false (case-insensitive).
 * @param use_regex      Forwarded to pattern construction; reserved for future
 *                       regex support and currently inert. Defaults to false.
 * @return A @ref query with the appropriate side(s) populated.
 *
 * @throws std::runtime_error if @p text is empty, contains no species, or is an
 *         arrow with no species on either side.
 *
 * @code
 * parse_query("CH4 + O2");        // any = {CH4, O2}, matched on either side
 * parse_query("CH4 => *OH");      // reactants = {CH4}, products = {*OH}
 * parse_query("O2 <=> O + O");    // reactants = {O2}, products = {O, O}
 * @endcode
 */
query parse_query(
    std::string_view text,
    bool case_sensitive = false,
    bool use_regex = false
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
