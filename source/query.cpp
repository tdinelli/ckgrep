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
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "query.hpp"
#include "string_utils.hpp"

namespace ckgrep {
// Split a side on '+' into pattern tokens. A leading stoichiometric
// coefficient (e.g. "2H") expands into that many repeated tokens, so a query
// of "2H" and "H+H" parse to the same query_side and match each other's
// reaction-side form.
query_side parse_side(std::string_view side, bool case_sensitive, bool use_regex) {
  query_side result;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= side.size(); ++i) {
    if (i == side.size() || side[i] == '+') {
      std::string_view tok = utils::trim(side.substr(start, i - start));
      if (!tok.empty()) {
        utils::coefficient_token parsed = utils::parse_coefficient(tok);
        std::string_view species = utils::trim(parsed.species);
        if (!species.empty()) {
          int count = utils::expansion_count(parsed.coefficient);
          for (int n = 0; n < count; ++n) {
            result.tokens.push_back(
                make_pattern(std::string(species), case_sensitive, use_regex)
            );
          }
        }
      }
      start = i + 1;
    }
  }
  return result;
}

query parse_query(std::string_view text, bool case_sensitive, bool use_regex) {
  std::string_view t = utils::trim(text);
  if (t.empty()) {
    throw std::runtime_error("empty query");
  }

  // Find an arrow, longest form first.
  std::size_t arrow_pos = std::string_view::npos;
  std::size_t arrow_len = 0;
  if (std::size_t p = t.find("<=>"); p != std::string_view::npos) {
    arrow_pos = p;
    arrow_len = 3;
  } else if (std::size_t p = t.find("=>"); p != std::string_view::npos) {
    arrow_pos = p;
    arrow_len = 2;
  } else if (std::size_t p = t.find('='); p != std::string_view::npos) {
    arrow_pos = p;
    arrow_len = 1;
  }

  query result;
  if (arrow_pos == std::string_view::npos) {
    // No arrow: either-side match.
    query_side side = parse_side(t, case_sensitive, use_regex);
    if (side.tokens.empty()) {
      throw std::runtime_error("query has no species");
    }
    result.any = std::move(side);
    return result;
  }

  std::string_view left = utils::trim(t.substr(0, arrow_pos));
  std::string_view right = utils::trim(t.substr(arrow_pos + arrow_len));

  if (!left.empty()) {
    query_side side = parse_side(left, case_sensitive, use_regex);
    if (!side.tokens.empty()) {
      result.reactants = std::move(side);
    }
  }
  if (!right.empty()) {
    query_side side = parse_side(right, case_sensitive, use_regex);
    if (!side.tokens.empty()) {
      result.products = std::move(side);
    }
  }
  if (!result.reactants && !result.products) {
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
