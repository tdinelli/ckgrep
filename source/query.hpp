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
#include "reaction_parser.hpp"

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
 * literal, glob.
 */
struct query_side {
  std::vector<std::unique_ptr<species_pattern>>
      tokens;  ///< AND-set of patterns for this side.
};

/**
 * @brief A reaction-level third-body constraint parsed from a query.
 *
 * @details Produced when the query text carries a third-body marker: a bare
 * "+M" token yields @ref kind == third_body_kind::mixture, a "(+...)" group
 * yields third_body_kind::falloff. Absence of a marker in the query means
 * "don't care" under match_mode::contains, and "the reaction must have no
 * third body" under match_mode::exact -- see third_body_matches().
 *
 * For fall-off constraints the collider follows chemical convention: a query
 * collider of "M" means the mixture itself, i.e. any fall-off reaction
 * matches (@ref any_collider). A specific collider like "(+N2)" is matched
 * against the reaction's collider through the same glob / case-sensitivity
 * pattern machinery used for species tokens, so "(+*)" also means any.
 */
struct third_body_constraint {
  third_body_kind kind =
      third_body_kind::mixture;  ///< mixture for "+M", falloff for "(+...)".
  bool any_collider =
      false;  ///< true when the query wrote "(+M)": any fall-off collider.
  std::unique_ptr<species_pattern> collider;  ///< Specific-collider pattern; null unless
                                              ///< kind == falloff and !any_collider.
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
 * At most one mode is active per query: either @ref any is set, or one/both
 * of @ref reactants and @ref products are set, never both modes at once. A
 * query whose only content is a third-body marker (e.g. "(+M)" to find every
 * fall-off reaction) sets @ref third_body alone, with no side populated.
 */
struct query {
  std::optional<query_side>
      reactants;  ///< Constraints left of the arrow; unset if absent.
  std::optional<query_side>
      products;                   ///< Constraints right of the arrow; unset if absent.
  std::optional<query_side> any;  ///< Arrow-less query, matched against either side.
  std::optional<third_body_constraint>
      third_body;  ///< Reaction-level third-body constraint; unset if absent.
};

/**
 * @brief Parses a query string into a @ref query.
 *
 * @details Recognizes the same reaction arrows as CHEMKIN mechanisms, tried
 * longest-form first so the shorter arrows do not match inside the longer ones:
 * `<=>`, then `=>`, then `=`. Text on each side of the arrow is split into
 * tokens on `+`, and each non-empty token is trimmed and handed to
 * make_pattern(). When no arrow is present the whole string is parsed as a
 * single either-side group (see @ref query::any). Third-body markers -- a
 * bare "M" token or a "(+...)" group -- are not species tokens: they become
 * the query's reaction-level @ref query::third_body constraint (see
 * third_body_constraint).
 *
 * The @p case_sensitive flag is forwarded unchanged to every pattern built for the
 * query, so a single query is uniformly case-sensitive (or not).
 *
 * When @p species_only is set, the text is a bare species pattern rather than
 * a reaction query: the reaction grammar (arrows, `+` joiners, third-body
 * markers) is skipped entirely, and the whole trimmed text becomes a single
 * pattern in @ref query::any via make_pattern(). Reaction-only syntax --
 * `=`, `+`, `<`, `>` -- has no meaning here and is rejected, since a species
 * name never legitimately contains it and letting it through would silently
 * fail to match anything instead of signaling the mistake.
 *
 * @param text           The raw query string, e.g. "CH4 + O2 => *OH".
 * @param case_sensitive When true, patterns require exact-case matching.
 *                       Defaults to false (case-insensitive).
 * @param species_only   When true, parse @p text as a bare species pattern
 *                       (see above) instead of a reaction query. Defaults to
 *                       false.
 * @return A @ref query with the appropriate side(s) and/or third-body
 *         constraint populated. Under @p species_only, only @ref query::any
 *         is set, holding exactly one pattern.
 *
 * @throws std::runtime_error if @p text is empty, contains neither a species
 *         nor a third-body marker, or (under @p species_only) contains
 *         reaction-only syntax.
 *
 * @code
 * parse_query("CH4 + O2");        // any = {CH4, O2}, matched on either side
 * parse_query("CH4 => *OH");      // reactants = {CH4}, products = {*OH}
 * parse_query("O2 <=> O + O");    // reactants = {O2}, products = {O, O}
 * parse_query("H+O2(+M)=");       // reactants = {H, O2}, any fall-off collider
 * parse_query("(+N2)");           // fall-off reactions with collider N2 only
 * parse_query("C3H5*", false, true);  // any = {C3H5*}, species-only mode
 * @endcode
 */
query parse_query(
    std::string_view text,
    bool case_sensitive = false,
    bool species_only = false
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
