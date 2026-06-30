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

#include <cctype>
#include <cmath>
#include <string>
#include <string_view>

namespace ckgrep::utils {
/**
 * @brief The set of characters treated as whitespace by trim().
 *
 * @details Space, tab, carriage return, and newline (" \t\r\n") -- the bytes
 * that can separate tokens or pad lines in a CHEMKIN mechanism file.
 */
inline constexpr std::string_view white_space = " \t\r\n";

/**
 * @brief Maximum distance from a whole number for expansion_count() to treat
 * a coefficient as an integer.
 *
 * @details Stoichiometric coefficients are parsed as doubles (see
 * parse_coefficient()), so a value that is conceptually "2" may arrive as
 * something like 1.9999999998 due to floating-point rounding. This tolerance
 * absorbs that imprecision when deciding whether a coefficient should expand
 * into repeated species occurrences.
 */
inline constexpr double coefficient_integer_tolerance = 1e-9;

/**
 * @brief Tests whether a glob pattern matches an entire string.
 *
 * @details Supports two wildcards: '*' matches any run of characters
 * (including none), and '?' matches exactly one character. All other
 * characters match literally. The match is anchored at both ends -- the
 * pattern must consume the whole of @p text, not just a prefix. A pattern
 * with no wildcards therefore behaves as a plain equality check.
 *
 * The implementation is the classic iterative two-pointer matcher with
 * backtracking on '*': when a tentative '*' match fails further along, it
 * rewinds and lets the star consume one additional character. This uses
 * O(1) extra space and no recursion. Worst-case time is O(n*m) in the
 * lengths of @p text and @p pattern, which is negligible for
 * species-name-length strings; the pathological inputs that approach that
 * bound (many '*' followed by near-misses) do not occur in practice here.
 *
 * Matching is case-sensitive: callers that need case-insensitive behavior
 * should normalize both arguments (e.g. via to_lower) before calling.
 *
 * @param pattern The glob pattern, possibly containing '*' and '?'.
 * @param text    The string to test in full against @p pattern.
 * @return true if @p pattern matches the entirety of @p text, false otherwise.
 *
 * @code
 * glob_match("C*",   "CH4");   // -> true  ('*' eats "H4")
 * glob_match("?H4",  "CH4");   // -> true  ('?' eats 'C')
 * glob_match("C*4",  "CHHH4"); // -> true  ('*' eats "HHH")
 * glob_match("C*4",  "CHHH");  // -> false (no trailing '4')
 * glob_match("O2",   "O2");    // -> true  (literal, no wildcards)
 * @endcode
 */
[[nodiscard]] constexpr bool inline glob_match(
    std::string_view pattern,
    std::string_view text
) {
  std::size_t p = 0;                          // cursor into pattern
  std::size_t t = 0;                          // cursor into text
  std::size_t star = std::string_view::npos;  // index of last '*' seen, or npos
  std::size_t match = 0;                      // text position to resume on backtrack

  while (t < text.size()) {
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
      // Literal or '?' match: consume one character from each.
      ++p;
      ++t;
    } else if (p < pattern.size() && pattern[p] == '*') {
      // Star: bookmark it and tentatively let it match zero characters.
      star = p++;
      match = t;
    } else if (star != std::string_view::npos) {
      // Mismatch, but a prior '*' can absorb one more character: backtrack.
      p = star + 1;
      t = ++match;
    } else {
      // Mismatch with no '*' to fall back on: no match.
      return false;
    }
  }

  // Any trailing '*' in the pattern matches the now-empty remainder.
  while (p < pattern.size() && pattern[p] == '*') {
    ++p;
  }
  return p == pattern.size();
}

/**
 * @brief Returns a lowercased copy of an ASCII string view.
 *
 * @details Folds ASCII uppercase letters ('A'-'Z') to lowercase; all other
 * bytes are copied unchanged. Unlike std::tolower this is locale-independent
 * and well-defined for every input byte, so results are stable regardless of
 * the global C locale. CHEMKIN species names are ASCII, so a plain 'A'-'Z'
 * fold is sufficient and avoids the undefined behavior std::tolower exhibits
 * on negative char values.
 *
 * This allocates: lowercasing produces characters not present in the source
 * buffer, so unlike trim() it cannot return a non-owning view and must return
 * an owned std::string.
 *
 * @param s The string view to lowercase.
 * @return A newly allocated std::string with every ASCII uppercase letter
 *         folded to lowercase.
 *
 * @code
 * to_lower("CH4 + O2");  // -> "ch4 + o2"
 * to_lower("End");       // -> "end"
 * to_lower("");          // -> "" (empty)
 * @endcode
 */
[[nodiscard]] inline std::string to_lower(std::string_view s) {
  std::string out(s);
  for (char& c : out) {
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c - 'A' + 'a');
    }
  }
  return out;
}

/**
 * @brief Case-insensitive substring test.
 *
 * @details Lowercases both @p haystack and @p needle (see to_lower()) and
 * checks containment on the results. Matches the case-insensitive-by-default
 * convention used for species pattern matching elsewhere in ckgrep, applied
 * here to plain comment-text search.
 *
 * @param haystack The text to search within.
 * @param needle   The text to search for.
 * @return true if @p needle occurs anywhere in @p haystack, ignoring case.
 *
 * @code
 * contains_ci("Soot precursor note", "PRECURSOR");  // -> true
 * contains_ci("Soot precursor note", "xyz");        // -> false
 * @endcode
 */
[[nodiscard]] inline bool
contains_ci(std::string_view haystack, std::string_view needle) {
  return to_lower(haystack).find(to_lower(needle)) != std::string::npos;
}

/**
 * @brief Removes leading and trailing whitespace from a string view.
 *
 * @details Strips spaces, tabs, carriage returns, and newlines (" \t\r\n")
 * from both ends of @p s. This is a non-owning operation: it returns a
 * sub-view into the same underlying buffer, performing no allocation
 * or copy. If @p s consists entirely of whitespace (or is empty), an
 * empty string_view is returned.
 *
 * @param s The string view to trim.
 * @return A string_view spanning the first to last non-whitespace
 *         character of @p s, or an empty string_view if @p s is
 *         all whitespace.
 *
 * @code
 * trim("   CH4 + O2  \r\n");  // -> "CH4 + O2"
 * trim("END");                // -> "END"
 * trim("   \t  ");            // -> "" (empty)
 * @endcode
 */
[[nodiscard]] constexpr std::string_view trim(std::string_view s) {
  const auto b = s.find_first_not_of(white_space);
  if (b == std::string_view::npos) {
    return {};
  }
  const auto e = s.find_last_not_of(white_space);
  return s.substr(b, e - b + 1);
}

/**
 * @brief Tests whether a byte is an ASCII decimal digit ('0'-'9').
 *
 * @details A constexpr-friendly replacement for std::isdigit, which takes an
 * int and is not specified as constexpr. Species and coefficient tokens are
 * always plain ASCII, so a direct range check is both sufficient and locale-
 * independent.
 *
 * @param c The byte to test.
 * @return true if @p c is '0'-'9', false otherwise.
 */
[[nodiscard]] constexpr bool is_ascii_digit(const char c) {
  return c >= '0' && c <= '9';
}

/**
 * @brief A token split into its leading stoichiometric coefficient and species name.
 *
 * @details Returned by parse_coefficient(). @ref coefficient is 1.0 when the
 * token carried no explicit coefficient (e.g. plain "H").
 */
struct coefficient_token {
  double coefficient = 1.0;  ///< Leading numeric coefficient; 1.0 if absent.
  std::string_view species;  ///< The token with the coefficient stripped and trimmed.
};

/**
 * @brief Splits a leading stoichiometric coefficient off a species token.
 *
 * @details Reads a run of digits and at most one '.' from the front of
 * @p token; if at least one digit was seen, the run (plus any whitespace
 * immediately after it) is treated as a coefficient and parsed with
 * std::stod, and @ref coefficient_token::species is the trimmed remainder.
 * If no digit is seen, or nothing non-numeric follows, @p token is returned
 * unchanged with a coefficient of 1.0 -- this is what makes a bare numeric
 * token (which is never a real species) pass through untouched rather than
 * being parsed as a coefficient with an empty species.
 *
 * @note A space between the coefficient and the species (e.g. "2 CH3") is not
 * strictly CHEMKIN-standard, but is accepted anyway so that loosely formatted
 * mechanism files, such as those emitted by OpenSMOKE, parse correctly too.
 *
 * @param token A single species-side token, e.g. "2H", "0.5O2", "2 CH3", "OH".
 * @return The parsed coefficient and the species text with it stripped.
 *
 * @code
 * parse_coefficient("2H");    // -> {2.0, "H"}
 * parse_coefficient("0.5O2"); // -> {0.5, "O2"}
 * parse_coefficient("2 CH3"); // -> {2.0, "CH3"}
 * parse_coefficient("OH");    // -> {1.0, "OH"}
 * @endcode
 */
[[nodiscard]] inline coefficient_token parse_coefficient(std::string_view token) {
  std::size_t i = 0;
  bool saw_digit = false;
  while (i < token.size() && (is_ascii_digit(token[i]) || token[i] == '.')) {
    if (is_ascii_digit(token[i])) {
      saw_digit = true;
    }
    ++i;
  }
  if (!saw_digit) {
    return {1.0, token};
  }

  std::string_view number = token.substr(0, i);
  std::string_view remainder = trim(token.substr(i));
  if (remainder.empty()) {
    // A bare number (no species after it) is not a coefficient + species pair.
    return {1.0, token};
  }

  return {std::stod(std::string(number)), remainder};
}

/**
 * @brief Number of discrete occurrences a stoichiometric coefficient expands to.
 *
 * @details A coefficient that is a whole number >= 1 (within floating-point
 * tolerance) expands to that many occurrences of the species, so that "2H"
 * and "H+H" are treated as equivalent on either side of a match. A
 * non-integer coefficient (e.g. 0.5) cannot expand into a whole number of
 * occurrences, so it is treated as exactly one occurrence -- the same as a
 * species with no coefficient at all.
 *
 * @param coefficient A stoichiometric coefficient, as parsed by parse_coefficient().
 * @return The number of occurrences to expand the species into.
 *
 * @code
 * expansion_count(2.0);  // -> 2
 * expansion_count(1.0);  // -> 1
 * expansion_count(0.5);  // -> 1
 * @endcode
 */
[[nodiscard]] inline int expansion_count(const double coefficient) {
  double rounded = std::round(coefficient);
  bool is_integer =
      std::abs(coefficient - rounded) < coefficient_integer_tolerance && rounded >= 1.0;
  return is_integer ? static_cast<int>(rounded) : 1;
}

/**
 * @brief Appends expansion_count(coefficient) copies of one species occurrence
 * to a caller-provided container.
 *
 * @details Both the query side (building pattern objects, see query.cpp) and
 * the reaction side (building plain species names, see matcher.cpp) need the
 * same "repeat this species N times per its coefficient" expansion; this
 * factors out that shared loop so each caller only supplies how to build one
 * occurrence.
 *
 * @tparam Container   A container supporting push_back, e.g. std::vector<T>.
 * @tparam MakeOne      Callable taking no arguments and returning one element
 *                      to push into @p out; invoked once per occurrence, so
 *                      it should be cheap to call repeatedly (e.g. a copy or
 *                      a small allocation), not something with side effects
 *                      that must run exactly once.
 * @param out          The container to append occurrences to.
 * @param coefficient  The stoichiometric coefficient (see parse_coefficient()).
 * @param make_one     Builds one element to append, called expansion_count()
 *                     times.
 *
 * @code
 * std::vector<std::string> names;
 * append_expanded(names, 2.0, [] { return std::string("H"); });  // -> {"H", "H"}
 * @endcode
 */
template <typename Container, typename MakeOne>
void append_expanded(Container& out, double coefficient, MakeOne&& make_one) {
  int count = expansion_count(coefficient);
  for (int i = 0; i < count; ++i) {
    out.push_back(make_one());
  }
}
}  // namespace ckgrep::utils
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
