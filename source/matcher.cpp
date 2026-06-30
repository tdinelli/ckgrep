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
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "matcher.hpp"
#include "string_utils.hpp"

namespace ckgrep {
std::vector<std::string> expand_species(const std::vector<species_amount>& side) {
  std::vector<std::string> expanded;
  expanded.reserve(side.size());
  for (const species_amount& sa : side) {
    utils::append_expanded(expanded, sa.coefficient, [&] { return sa.name; });
  }
  return expanded;
}

bool find_augmenting_path(
    const query_side& side,
    const std::vector<std::string>& species,
    int token,
    std::vector<int>& assigned_to,
    std::vector<char>& seen
) {
  for (std::size_t s = 0; s < species.size(); ++s) {
    if (seen[s] != 0) {
      continue;
    }
    if (!side.tokens[token]->matches(species[s])) {
      continue;
    }
    seen[s] = 1;
    if (assigned_to[s] == -1 ||
        find_augmenting_path(side, species, assigned_to[s], assigned_to, seen)) {
      assigned_to[s] = token;
      return true;
    }
  }
  return false;
}

bool can_assign(const query_side& side, const std::vector<std::string>& species) {
  const std::size_t token_count = side.tokens.size();
  if (token_count > species.size()) {
    return false;
  }

  // assigned_to[s] = index of the token currently using species s, or -1.
  std::vector<int> assigned_to(species.size(), -1);

  // `seen` marks species visited during the search for ONE top-level token. It
  // must be shared across the augmenting-path recursion (Kuhn's algorithm) and
  // reset once per top-level token -- not per recursive call.
  std::vector<char> seen(species.size(), 0);

  for (std::size_t token = 0; token < token_count; ++token) {
    std::fill(seen.begin(), seen.end(), 0);
    if (!find_augmenting_path(
            side,
            species,
            static_cast<int>(token),
            assigned_to,
            seen
        )) {
      return false;
    }
  }
  return true;
}

bool side_matches(
    const query_side& side,
    const std::vector<species_amount>& species,
    match_mode mode
) {
  std::vector<std::string> expanded = expand_species(species);
  if (!can_assign(side, expanded)) {
    return false;
  }
  if (mode == match_mode::exact && side.tokens.size() != expanded.size()) {
    return false;
  }
  return true;
}

bool matches(const query& q, const reaction& r, match_mode mode) {
  if (q.any) {
    return side_matches(*q.any, r.reactants, mode) ||
           side_matches(*q.any, r.products, mode);
  }

  bool ok = true;
  if (q.reactants) {
    ok = ok && side_matches(*q.reactants, r.reactants, mode);
  }
  if (q.products) {
    ok = ok && side_matches(*q.products, r.products, mode);
  }
  return ok;
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
