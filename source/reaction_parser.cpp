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
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "reaction_parser.hpp"
#include "string_utils.hpp"

namespace ckgrep {
arrow_info find_arrow(std::string_view t) {
  if (std::size_t p = t.find("<=>"); p != std::string_view::npos) {
    return {p, 3, true};
  }
  if (std::size_t p = t.find("=>"); p != std::string_view::npos) {
    return {p, 2, false};
  }
  if (std::size_t p = t.find('='); p != std::string_view::npos) {
    return {p, 1, true};
  }
  return {};
}

falloff_extraction extract_falloff_markers(std::string_view side) {
  falloff_extraction result;
  result.cleaned.reserve(side.size());
  for (std::size_t i = 0; i < side.size();) {
    if (side[i] != '(') {
      result.cleaned.push_back(side[i]);
      ++i;
      continue;
    }
    std::size_t close = side.find(')', i);
    std::size_t end = (close == std::string_view::npos) ? side.size() : close + 1;
    std::string_view inside = (close == std::string_view::npos)
                                  ? side.substr(i + 1)
                                  : side.substr(i + 1, close - i - 1);
    if (std::string_view inner = utils::trim(inside);
        !inner.empty() && inner.front() == '+') {
      // Fall-off marker: drop it from the text, keep the collider it names.
      if (result.collider.empty()) {
        result.collider = std::string(utils::trim(inner.substr(1)));
      }
      i = end;
      continue;
    }
    result.cleaned.append(side.substr(i, end - i));
    i = end;
  }
  return result;
}

std::optional<double> parse_arrhenius_value(std::string_view token) {
  // Copies only the single token (unavoidable: strtod needs a null-terminated
  // buffer, and the D/d substitution needs a mutable one), not the whole line.
  std::string normalized(token);
  for (char& c : normalized) {
    if (c == 'D') {
      c = 'E';
    } else if (c == 'd') {
      c = 'e';
    }
  }

  const char* const begin = normalized.c_str();
  const char* const expected_end = begin + normalized.size();
  char* actual_end = nullptr;
  errno = 0;
  const double value = std::strtod(begin, &actual_end);

  const bool consumed_nothing = (actual_end == begin);
  const bool trailing_junk = (actual_end != expected_end);
  const bool out_of_range = (errno == ERANGE);
  if (consumed_nothing || trailing_junk || out_of_range) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::array<double, 3>> parse_arrhenius(std::string_view rest) {
  const std::vector<std::string_view> parts = utils::split_whitespace(rest);
  if (parts.size() < 3) {
    return std::nullopt;
  }
  const std::optional<double> A = parse_arrhenius_value(parts[parts.size() - 3]);
  const std::optional<double> n = parse_arrhenius_value(parts[parts.size() - 2]);
  const std::optional<double> Ea = parse_arrhenius_value(parts[parts.size() - 1]);
  if (!A || !n || !Ea) {
    return std::nullopt;
  }
  return std::array<double, 3>{*A, *n, *Ea};
}

parsed_side parse_reaction_side(std::string_view side) {
  falloff_extraction extraction = extract_falloff_markers(side);
  const std::string_view text = extraction.cleaned;

  parsed_side result;
  if (!extraction.collider.empty()) {
    result.third_body = third_body_kind::falloff;
    result.collider = std::move(extraction.collider);
  }

  std::size_t start = 0;
  for (std::size_t i = 0; i <= text.size(); ++i) {
    if (i != text.size() && text[i] != '+') {
      continue;
    }
    std::string_view token = utils::trim(text.substr(start, i - start));
    start = i + 1;
    if (token.empty()) {
      continue;
    }
    utils::coefficient_token parsed = utils::parse_coefficient(token);
    std::string_view name = utils::trim(parsed.species);
    if (name.empty()) {
      continue;
    }
    // Bare third-body 'M' is a property of the reaction, not a species. A
    // "(+...)" marker is the stronger, pressure-dependent form; keep it if
    // both appear on one side.
    if (name == "M" || name == "m") {
      if (result.third_body == third_body_kind::none) {
        result.third_body = third_body_kind::mixture;
        result.collider = "M";
      }
      continue;
    }
    parsed_species sp;
    sp.name = std::string(name);
    sp.coefficient = parsed.coefficient;
    sp.expanded.assign(
        static_cast<std::size_t>(utils::expansion_count(parsed.coefficient)),
        sp.name
    );
    result.species.push_back(std::move(sp));
  }
  return result;
}

std::optional<parsed_reaction> parse_reaction_line(std::string_view line) {
  // Strip trailing comment (everything from '!' onward), then trim.
  std::string_view t = line;
  if (std::size_t bang = t.find('!'); bang != std::string_view::npos) {
    t = t.substr(0, bang);
  }
  t = utils::trim(t);
  if (t.empty()) {
    return std::nullopt;
  }

  const arrow_info arrow = find_arrow(t);
  if (arrow.pos == std::string_view::npos) {
    return std::nullopt;
  }

  const std::string_view left = t.substr(0, arrow.pos);
  std::string_view rest = t.substr(arrow.pos + arrow.len);

  parsed_reaction r;
  r.raw = std::string(t);
  r.reversible = arrow.reversible;

  if (std::optional<std::array<double, 3>> rate = parse_arrhenius(rest)) {
    r.arrhenius = *rate;
    // The rate tokens are views into rest, so the products end where the
    // first of the three begins.
    const std::vector<std::string_view> parts = utils::split_whitespace(rest);
    const char* tail = parts[parts.size() - 3].data();
    rest = rest.substr(0, static_cast<std::size_t>(tail - rest.data()));
  }

  parsed_side reactants = parse_reaction_side(left);
  parsed_side products = parse_reaction_side(rest);
  if (reactants.species.empty() && products.species.empty()) {
    return std::nullopt;
  }

  // The third body is a property of the whole reaction; CHEMKIN writes the
  // marker on both sides. Prefer the stronger fall-off form if a sloppy file
  // makes the sides disagree.
  if (reactants.third_body == third_body_kind::falloff ||
      products.third_body == third_body_kind::none) {
    r.third_body = reactants.third_body;
    r.collider = std::move(reactants.collider);
  } else {
    r.third_body = products.third_body;
    r.collider = std::move(products.collider);
  }
  r.reactants = std::move(reactants.species);
  r.products = std::move(products.species);
  return r;
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
