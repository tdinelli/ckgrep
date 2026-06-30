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

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ckgrep {

/**
 * @brief One comment found while scanning a mechanism file.
 *
 * @details Covers both full-line comments ("! note") and inline trailing
 * comments ("H+H=H2 1e+14 0 0  ! note"); both are produced by scan_comments().
 */
struct comment {
  std::string raw;              ///< Original line, for output.
  std::size_t line_number = 0;  ///< 1-based line in the source file.
  std::string text;  ///< Comment text, '!' and surrounding whitespace stripped.
};

/**
 * @brief One species on a reaction side together with its stoichiometric coefficient.
 *
 * @details E.g. "2H" parses to {"H", 2.0}. @ref coefficient is 1.0 when the
 * reaction wrote no explicit coefficient for that species.
 */
struct species_amount {
  std::string name;          ///< Species name, third-body markers stripped.
  double coefficient = 1.0;  ///< Leading numeric coefficient; 1.0 if absent.
};

/**
 * @brief One parsed reaction, reduced to just what search needs.
 *
 * @details Holds the original text plus the two species multisets. Kinetics
 * (A/b/Ea, PLOG, LOW, TROE, third-body efficiencies) are intentionally
 * ignored -- this is a search index, not a mechanism loader.
 */
struct reaction {
  std::string raw;                        ///< Original reaction line, for output.
  std::size_t line_number = 0;            ///< 1-based line in the source file.
  std::vector<species_amount> reactants;  ///< Species + coeffs; (+M)/M stripped.
  std::vector<species_amount> products;   ///< Species + coeffs; (+M)/M stripped.
  std::string reaction_type;              ///< Reserved for future classification.
  bool reversible = true;                 ///< true for <=> or =, false for =>.
};

/**
 * @brief Tests whether a case-insensitive prefix matches the start of a string.
 *
 * @details Used by is_reaction_line() to recognize CHEMKIN keyword/sub-data
 * lines (e.g. "LOW", "TROE") regardless of casing, without allocating: only
 * the leading @c prefix.size() bytes of @p s are compared.
 *
 * @param s      The string to test.
 * @param prefix The prefix to look for, compared case-insensitively.
 * @return true if @p s is at least as long as @p prefix and starts with it,
 *         ignoring case.
 */
bool starts_with_ci(std::string_view s, std::string_view prefix);

/**
 * @brief Drops "(+...)" fall-off markers like (+M) or (+N2) from a reaction side.
 *
 * @details Other parenthesized text is kept verbatim (unusual species names
 * may contain parens). The '+' inside "(+M)" must not be treated as a
 * species separator, so these groups are stripped before split_side() splits
 * on '+'.
 *
 * @param side One reaction side, e.g. "H+O2(+M)".
 * @return @p side with every "(+...)" fall-off marker removed.
 */
std::string remove_falloff_markers(std::string_view side);

/**
 * @brief Splits one reaction side into species_amount entries.
 *
 * @details Strips fall-off markers (see remove_falloff_markers()), splits
 * the remainder on '+', and parses each token's leading stoichiometric
 * coefficient (see utils::parse_coefficient()). Blank tokens and the bare
 * third-body marker ("M"/"m") are dropped, not appended to @p out.
 *
 * @param side     One reaction side, e.g. "2H+O2(+M)".
 * @param[out] out Species entries are appended here, in left-to-right order.
 */
void split_side(std::string_view side, std::vector<species_amount>& out);

/**
 * @brief Location and kind of a direction arrow found in a reaction string.
 *
 * @details Returned by find_arrow(). @ref pos is std::string_view::npos when
 * no arrow was found, in which case @ref len and @ref reversible are
 * meaningless.
 */
struct arrow_info {
  std::size_t pos = std::string_view::npos;  ///< Index where the arrow starts, or npos.
  std::size_t len = 0;                       ///< Length of the arrow text (1, 2, or 3).
  bool reversible = true;                    ///< true for `<=>` or `=`, false for `=>`.
};

/**
 * @brief Finds the direction arrow in a reaction string.
 *
 * @details Tries the longest/most specific form first -- `<=>`, then `=>`,
 * then `=` -- so the shorter arrows do not match inside the longer ones.
 *
 * @param t The text to search, typically a trimmed reaction line.
 * @return The first matching arrow's location and kind, or a default-constructed
 *         arrow_info (pos == std::string_view::npos) if none was found.
 */
arrow_info find_arrow(std::string_view t);

/**
 * @brief Finds the start offset of the trailing rate-coefficient tokens.
 *
 * @details A CHEMKIN reaction line always ends with exactly three
 * whitespace-delimited rate-coefficient tokens (A, n, Ea), regardless of how
 * A is written -- "174", "2277000000000000.0", and "6.1400e+05" are all
 * valid. Trying to classify a token as "numeric" up front is fragile:
 * scientific notation can itself contain a '+' (the exponent sign), which is
 * indistinguishable from a species-joining '+' by looking at one token
 * alone. Anchoring on a fixed count from the end of the line sidesteps that
 * ambiguity entirely: whatever the species text looks like, the products
 * always end where the last three whitespace-delimited tokens begin.
 *
 * @param rest The text after the direction arrow (products + rate tail).
 * @return The start offset of the first of the three trailing tokens, i.e.
 *         the products text is rest.substr(0, that offset). If fewer than
 *         three whitespace-delimited tokens exist, returns rest.size() (no
 *         rate tail to strip).
 */
std::size_t rate_tail_start(std::string_view rest);

/**
 * @brief Extracts the product portion of the text after a reaction's arrow.
 *
 * @details Everything before the trailing rate-coefficient tokens (see
 * rate_tail_start()).
 *
 * @param rest The text after the direction arrow (products + rate tail).
 * @return The product-side text, with the trailing rate tail removed.
 */
std::string_view product_portion(std::string_view rest);

/**
 * @brief Walks text one physical line at a time, invoking a callback per line.
 *
 * @details Splits @p text on '\\n' and calls `fn(line, line_number)` once per
 * physical line (1-based), including a trailing line with no terminating
 * newline. Shared by scan_reactions() and scan_comments() so both see the
 * exact same line/line-number splitting.
 *
 * @tparam Fn        Callable taking (std::string_view line, std::size_t line_number).
 * @param text       The full text of a mechanism file.
 * @param fn         Invoked once per physical line.
 */
template <class Fn>
void for_each_line(std::string_view text, Fn&& fn) {
  std::size_t line_number = 0;
  std::size_t pos = 0;

  while (pos <= text.size()) {
    std::size_t nl = text.find('\n', pos);
    std::string_view line = text.substr(
        pos,
        nl == std::string_view::npos ? std::string_view::npos : nl - pos
    );
    ++line_number;

    fn(line, line_number);

    if (nl == std::string_view::npos) {
      break;
    }
    pos = nl + 1;
  }
}

/**
 * @brief Decides whether a physical line is a reaction line.
 *
 * @details A line counts as a reaction line if it is non-blank, is not a
 * comment (does not start with '!'), is not a known CHEMKIN keyword or
 * sub-data line (LOW, TROE, PLOG, ...), and contains a direction arrow
 * (`<=>`, `=>`, or `=`).
 *
 * @param line One physical line, not yet trimmed.
 * @return true if @p line looks like a reaction line.
 */
bool is_reaction_line(std::string_view line);

/**
 * @brief Splits one reaction line into a @ref reaction.
 *
 * @details Handles order-independent sides, stoichiometric coefficients
 * (2CH3, 2 CH3), (+M)/(+species) fall-off markers, bare M third body, and
 * trailing rate coefficients after the equation.
 *
 * @param line        One physical line, not yet trimmed.
 * @param line_number 1-based line number, stored on @p out for output.
 * @param[out] out    Populated with the parsed reaction on success; left
 *                     untouched on failure.
 * @return true if @p line is a parseable reaction, false otherwise.
 */
bool parse_reaction_line(std::string_view line, std::size_t line_number, reaction& out);

/**
 * @brief Scans a whole mechanism/text buffer and returns every reaction found.
 *
 * @details Stops nothing early; comments and keyword blocks are skipped, not
 * treated as errors.
 *
 * @param text The full text of a mechanism file.
 * @return Every reaction found, in source order.
 */
std::vector<reaction> scan_reactions(std::string_view text);

/**
 * @brief Scans a whole mechanism/text buffer and returns every comment found.
 *
 * @details Covers both full-line ("! note") and inline trailing
 * ("H+H=H2 1e+14 0 0  ! note") comments.
 *
 * @param text The full text of a mechanism file.
 * @return Every comment found, in source order.
 */
std::vector<comment> scan_comments(std::string_view text);

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
