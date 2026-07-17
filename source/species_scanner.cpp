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
#include <string>
#include <string_view>
#include <vector>

#include "species_scanner.hpp"
#include "string_utils.hpp"

namespace ckgrep {
// Splits a line at its first '!' into the live and comment parts, the same
// way search_text() does for reaction lines. The comment part still carries
// its leading '!'s so double-commented lines ("!!CH4 ...") are handled
// identically to reaction_scanner: callers strip them before use.
struct line_parts {
  std::string_view live;
  std::string_view comment;  // empty when the line has no '!'
};

line_parts split_comment(std::string_view line) {
  const std::size_t bang = line.find('!');
  if (bang == std::string_view::npos) {
    return {line, {}};
  }
  return {line.substr(0, bang), line.substr(bang)};
}

// The body of a commented-out line, with its leading run of '!' stripped and
// truncated at any further '!' -- mirrors reaction_scanner's handling of
// "!!species ..." style double comments.
std::string_view comment_body(std::string_view comment) {
  if (comment.empty()) {
    return {};
  }
  std::size_t body = comment.find_first_not_of('!');
  if (body == std::string_view::npos) {
    return {};
  }
  std::string_view rest = comment.substr(body);
  return rest.substr(0, rest.find('!'));
}

void match_tokens(
    std::string_view line,
    const species_pattern& pattern,
    std::size_t line_number,
    std::vector<search_hit>& hits
) {
  for (std::string_view token : utils::split_whitespace(line)) {
    if (pattern.matches(token)) {
      hits.push_back({line_number, std::string(token)});
    }
  }
}

// A SPECIES/SPEC block starts on a line beginning with one of these keywords
// (case-insensitive) and runs until a line beginning with END.
bool starts_species_block(std::string_view line) {
  std::string_view t = utils::trim(line);
  return utils::starts_with_ci(t, "SPECIES") || utils::starts_with_ci(t, "SPEC");
}

bool is_end_keyword(std::string_view line) {
  return utils::starts_with_ci(utils::trim(line), "END");
}

std::vector<search_hit> scan_species_block(
    std::string_view text,
    const species_pattern& pattern,
    const search_options& opts
) {
  std::vector<search_hit> hits;
  bool in_block = false;

  for_each_line(text, [&](std::string_view line, std::size_t line_number) {
    const line_parts parts = split_comment(line);

    if (!in_block) {
      if (starts_species_block(parts.live)) {
        in_block = true;
      }
      return;
    }
    if (is_end_keyword(parts.live)) {
      in_block = false;
      return;
    }

    match_tokens(parts.live, pattern, line_number, hits);
    if (opts.search_comments) {
      match_tokens(comment_body(parts.comment), pattern, line_number, hits);
    }
  });

  return hits;
}

// A thermo header line ends with the digit '1' (the classic NASA-polynomial
// line-position marker), optionally followed by '&' when the elemental
// composition overflows onto its own continuation line rather than being
// packed into the header. Lines 2-4 of an entry end with '2', '3', '4'
// respectively, by the same convention.
bool ends_with_marker(std::string_view t, char marker) {
  if (!t.empty() && t.back() == '&') {
    t.remove_suffix(1);
    t = utils::trim(t);
  }
  return !t.empty() && t.back() == marker;
}

bool is_thermo_header(std::string_view line) {
  return ends_with_marker(utils::trim(line), '1');
}

bool is_thermo_coefficient_line(std::string_view line) {
  std::string_view t = utils::trim(line);
  return !t.empty() && (t.back() == '2' || t.back() == '3' || t.back() == '4');
}

// The species name on a thermo header line is its first whitespace token.
std::string_view thermo_header_name(std::string_view line) {
  auto tokens = utils::split_whitespace(line);
  return tokens.empty() ? std::string_view{} : tokens.front();
}

// A THERMO/THER block starts on a line beginning with one of these keywords
// (case-insensitive) and runs until a line beginning with END.
bool starts_thermo_block(std::string_view line) {
  std::string_view t = utils::trim(line);
  return utils::starts_with_ci(t, "THERMO") || utils::starts_with_ci(t, "THER");
}

std::vector<search_hit> scan_thermo_block(
    std::string_view text,
    const species_pattern& pattern,
    const search_options& opts
) {
  std::vector<search_hit> hits;
  bool in_block = false;

  auto try_header = [&](std::string_view line, std::size_t line_number) {
    if (!is_thermo_header(line)) {
      return;
    }
    std::string_view name = thermo_header_name(line);
    if (!name.empty() && pattern.matches(name)) {
      hits.push_back({line_number, std::string(name)});
    }
  };

  for_each_line(text, [&](std::string_view line, std::size_t line_number) {
    const line_parts parts = split_comment(line);

    if (!in_block) {
      if (starts_thermo_block(parts.live)) {
        in_block = true;
      }
      return;
    }
    if (is_end_keyword(parts.live)) {
      in_block = false;
      return;
    }

    // A composition-continuation or coefficient line is not a header and
    // carries no species name; skip it without inspection either way.
    if (is_thermo_coefficient_line(parts.live)) {
      return;
    }

    try_header(parts.live, line_number);
    if (opts.search_comments) {
      std::string_view body = comment_body(parts.comment);
      if (!is_thermo_coefficient_line(body)) {
        try_header(body, line_number);
      }
    }
  });

  return hits;
}

std::vector<search_hit> scan_transport_lines(
    std::string_view text,
    const species_pattern& pattern,
    const search_options& opts
) {
  std::vector<search_hit> hits;

  auto try_line = [&](std::string_view line, std::size_t line_number) {
    auto tokens = utils::split_whitespace(line);
    if (tokens.empty()) {
      return;
    }
    std::string_view name = tokens.front();
    if (pattern.matches(name)) {
      hits.push_back({line_number, std::string(name)});
    }
  };

  for_each_line(text, [&](std::string_view line, std::size_t line_number) {
    const line_parts parts = split_comment(line);
    try_line(parts.live, line_number);
    if (opts.search_comments) {
      try_line(comment_body(parts.comment), line_number);
    }
  });

  return hits;
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
