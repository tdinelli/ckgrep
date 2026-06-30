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

#include <string>
#include <vector>

#include "query.hpp"
#include "reaction_scanner.hpp"

namespace ckgrep {
/**
 * @brief How strictly a query side must line up with a reaction side.
 */
enum class match_mode {
  contains,  ///< Every query token matches a distinct species; extras allowed.
  exact      ///< As contains, plus no leftover species on the reaction side.
};

/**
 * @brief Expands a reaction side's species_amount list into one name per occurrence.
 *
 * @details A species with an integer coefficient >= 1 (e.g. "2H") contributes
 * that many entries, so a query of "H+H" matches a reaction side written as
 * "2H" and vice versa. A non-integer coefficient (e.g. "0.5O2") contributes
 * exactly one entry, the same as a species with no coefficient at all. See
 * utils::append_expanded() for the underlying expansion rule.
 *
 * @param side One side (reactants or products) of a parsed reaction.
 * @return One name per occurrence, e.g. {"H", "O2"} expands "2H" to
 *         {"H", "H", "O2"}.
 */
std::vector<std::string> expand_species(const std::vector<species_amount>& side);

/**
 * @brief Tests whether one reaction side satisfies one query side.
 *
 * @details Expands @p species by coefficient (see expand_species()) and
 * checks that every token in @p side can be assigned a distinct expanded
 * occurrence (see can_assign()). Under match_mode::exact, the expanded side
 * must additionally have no leftover, unmatched occurrences -- i.e. the
 * token count and the expanded species count must be equal.
 *
 * @param side    The query side to test.
 * @param species One side (reactants or products) of a reaction.
 * @param mode    How strictly the two must line up; see match_mode.
 * @return true if @p species satisfies @p side under @p mode, false otherwise.
 */
bool side_matches(
    const query_side& side,
    const std::vector<species_amount>& species,
    match_mode mode
);

/**
 * @brief Tests whether every query token can be assigned to a distinct species.
 *
 * @details Each token in @p side must match a different entry in @p species --
 * no two tokens may claim the same species occurrence. This is bipartite
 * matching (query tokens vs. species occurrences, edges = species_pattern::matches())
 * solved via Kuhn's algorithm with augmenting paths: for each token, search
 * for a species it matches that is either free or whose current owner can be
 * reassigned to a different species. Sides have on the order of 2-4 species,
 * so the naive O(tokens * species^2) bound here is negligible in practice.
 *
 * @param side    The query side whose tokens need an assignment.
 * @param species Species occurrences to assign tokens to (already expanded by
 *                coefficient -- see utils::append_expanded()), one entry per
 *                occurrence, e.g. "2H" expands to {"H", "H"}.
 * @return true if an injective assignment exists (every token gets a distinct
 *         species), false otherwise.
 */
bool can_assign(const query_side& side, const std::vector<std::string>& species);

/**
 * @brief Searches for an augmenting path that lets @p token claim a species.
 *
 * @details The recursive step of Kuhn's algorithm, called once per top-level
 * token by can_assign(). Tries each unvisited species @p token matches; if
 * that species is free, @p token claims it directly. If it is already
 * claimed by another token, recursively tries to move that token onto a
 * different species, freeing the contested one up for @p token. @p seen
 * prevents revisiting a species within the same top-level search and must be
 * reset by the caller before each top-level token, not before each
 * recursive call -- that is what lets the recursion explore alternative
 * reassignments instead of looping.
 *
 * @param side        The query side whose tokens are being assigned.
 * @param species     Species occurrences being assigned to (see can_assign()).
 * @param token       Index into side.tokens of the token seeking a species.
 * @param[in,out] assigned_to assigned_to[s] is the index of the token
 *                    currently claiming species s, or -1 if species s is free.
 *                    Updated in place as the search succeeds.
 * @param[in,out] seen seen[s] is true once species s has been visited during
 *                    this top-level search.
 * @return true if @p token (directly or by displacing another token) was
 *         assigned a species, false if no augmenting path exists.
 */
bool find_augmenting_path(
    const query_side& side,
    const std::vector<std::string>& species,
    int token,
    std::vector<int>& assigned_to,
    std::vector<char>& seen
);

/**
 * @brief Tests whether a reaction satisfies a query under a given match mode.
 *
 * @details query::reactants and query::products, when set, anchor to the
 * matching reaction side: each side present in @p q must independently
 * satisfy side_matches() against the corresponding side of @p r (an AND
 * across the sides that were constrained). query::any, when set instead,
 * is tested against the reactants OR the products of @p r (either-side
 * default, see query for which mode a parsed query is in).
 *
 * @param q    The parsed query to test.
 * @param r    The reaction to test it against.
 * @param mode How strictly each side must line up; see match_mode.
 * @return true if @p r satisfies @p q under @p mode, false otherwise.
 */
bool matches(const query& q, const reaction& r, match_mode mode);
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
