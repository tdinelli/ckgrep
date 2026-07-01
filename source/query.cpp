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
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "query.hpp"
#include "reaction_parser.hpp"
#include "string_utils.hpp"

namespace ckgrep {
// Build a query_side from a list of parsed species, turning each expanded
// occurrence into a pattern. A leading stoichiometric coefficient (e.g. "2H")
// expands into that many repeated tokens, so a query of "2H" and "H+H" parse
// to the same query_side and match each other's reaction-side form.
query_side make_query_side(
    const std::vector<parsed_species>& species,
    bool case_sensitive
) {
  query_side result;
  for (const parsed_species& sp : species) {
    for (const std::string& name : sp.expanded) {
      result.tokens.push_back(make_pattern(name, case_sensitive));
    }
  }
  return result;
}

// Merge one parsed side's third-body marker into the query-level constraint.
// CHEMKIN writes the marker on both sides, so the sides of a well-formed
// query agree; the stronger fall-off form wins if they do not. Following
// chemical convention, a fall-off collider of "M" means the mixture itself,
// so "(+M)" constrains to any fall-off reaction; a specific collider such as
// "(+N2)" becomes a pattern and goes through the same glob/case machinery as
// species tokens.
void merge_third_body(query& result, const parsed_side& side, bool case_sensitive) {
  if (side.third_body == third_body_kind::none) {
    return;
  }
  if (result.third_body && result.third_body->kind == third_body_kind::falloff) {
    return;
  }
  third_body_constraint constraint;
  constraint.kind = side.third_body;
  if (side.third_body == third_body_kind::falloff) {
    if (utils::to_lower(side.collider) == "m") {
      constraint.any_collider = true;
    } else {
      constraint.collider = make_pattern(side.collider, case_sensitive);
    }
  }
  result.third_body = std::move(constraint);
}

query parse_query(std::string_view text, bool case_sensitive) {
  std::string_view t = utils::trim(text);
  if (t.empty()) {
    throw std::runtime_error("empty query");
  }

  // The query grammar is the reaction grammar without the trailing rate
  // coefficients, so parsing composes the same primitives a mechanism line
  // goes through -- find_arrow() and parse_reaction_side() -- but never the
  // rate-tail handling: in "O2 = O + O" the trailing species must not be
  // mistaken for rate coefficients. Third-body markers become reaction-level
  // constraints rather than species tokens.
  query result;
  const arrow_info arrow = find_arrow(t);

  if (arrow.pos == std::string_view::npos) {
    parsed_side side = parse_reaction_side(t);
    merge_third_body(result, side, case_sensitive);
    if (!side.species.empty()) {
      result.any = make_query_side(side.species, case_sensitive);
    }
    if (!result.any && !result.third_body) {
      throw std::runtime_error("query has no species");
    }
    return result;
  }

  parsed_side left = parse_reaction_side(t.substr(0, arrow.pos));
  parsed_side right = parse_reaction_side(t.substr(arrow.pos + arrow.len));
  merge_third_body(result, left, case_sensitive);
  merge_third_body(result, right, case_sensitive);

  if (!left.species.empty()) {
    result.reactants = make_query_side(left.species, case_sensitive);
  }
  if (!right.species.empty()) {
    result.products = make_query_side(right.species, case_sensitive);
  }
  if (!result.reactants && !result.products && !result.third_body) {
    throw std::runtime_error("query has no species on either side");
  }
  return result;
}
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
