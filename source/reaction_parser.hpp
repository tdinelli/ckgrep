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

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ckgrep {
/**
 * @brief The kind of third body a reaction (or a query) carries.
 *
 * @details CHEMKIN distinguishes three chemically different situations that
 * look deceptively similar in the text:
 *
 * - @ref none:    "H+O2=HO2" -- a plain reaction.
 * - @ref mixture: "H+O2+M=HO2+M" -- a third-body reaction; any species in the
 *                 mixture (M) carries away the excess energy.
 * - @ref falloff: "H+O2(+M)=HO2(+M)" or "(+N2)" -- a pressure-dependent
 *                 fall-off reaction, optionally with a specific collider.
 *
 * The marker is stripped from the species lists (M is never a species), but
 * the kind is preserved so searches can distinguish the three forms.
 */
enum class third_body_kind {
  none,     ///< No third body.
  mixture,  ///< Bare "+M": generic third-body reaction.
  falloff,  ///< "(+M)" / "(+N2)": pressure-dependent fall-off reaction.
};

/**
 * @brief One species entry on a reaction side.
 *
 * @details Holds the original coefficient as written in the source, plus an
 * already-expanded flat list of occurrences. The expansion follows the rule in
 * utils::expansion_count(): an integer coefficient N >= 1 produces N entries in
 * @ref expanded, while a non-integer coefficient (e.g. 0.5) produces exactly
 * one. This means "2H" and "H+H" yield identical @ref expanded lists and are
 * therefore interchangeable for matching.
 *
 * @note @ref coefficient is the raw value parsed from the source and is kept
 * for informational purposes. Matching always operates on @ref expanded.
 */
struct parsed_species {
  std::string name;          ///< Species name, third-body markers stripped.
  double coefficient = 1.0;  ///< Stoichiometric coefficient as written; 1.0 if absent.
  std::vector<std::string>
      expanded;  ///< One name per occurrence after coefficient expansion.
};

/**
 * @brief One parsed reaction side: its species plus the third-body marker
 * found on it, if any.
 *
 * @details Returned by parse_reaction_side(). The third-body information is
 * reported per side because that is where it appears in the text; a whole
 * reaction combines the two sides' findings (see parse_reaction_line()).
 */
struct parsed_side {
  std::vector<parsed_species> species;  ///< Species entries, left-to-right.
  third_body_kind third_body =
      third_body_kind::none;  ///< Marker found on this side, if any.
  std::string collider;  ///< "M", or the specific species of "(+N2)"; empty when none.
};

/**
 * @brief One fully parsed reaction.
 *
 * @details Holds the structural information extracted from a single CHEMKIN
 * reaction line: the two species sides, the direction arrow kind, the
 * third-body classification, the raw Arrhenius rate coefficients, and the
 * original text for output. Kinetic sub-data (PLOG, LOW, TROE, third-body
 * efficiencies) are intentionally ignored, this is a search index, not a
 * mechanism loader.
 *
 * @note @ref arrhenius stores [A, n, Ea] in that order, matching the CHEMKIN
 * convention. The field is empty when the line carried no parseable rate
 * tail. @ref line_number is not known to the parser; scan_reactions() stamps
 * it after a successful parse.
 */
struct parsed_reaction {
  std::string raw;                        ///< Original line text, for output.
  std::size_t line_number = 0;            ///< 1-based line in the source file.
  bool reversible = true;                 ///< true for <=> or =, false for =>.
  std::vector<parsed_species> reactants;  ///< Species entries for the left side.
  std::vector<parsed_species> products;   ///< Species entries for the right side.
  third_body_kind third_body =
      third_body_kind::none;  ///< Third-body classification of the reaction.
  std::string collider;       ///< Fall-off collider ("M", "N2", ...); empty when none.
  std::optional<std::array<double, 3>> arrhenius;  ///< [A, n, Ea]; empty if absent.
};

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
 * CHEMKIN semantics: `=` and `<=>` both denote a reversible reaction (`<=>`
 * is just the more explicit spelling); `=>` denotes an irreversible,
 * forward-only one.
 *
 * @param t The text to search, typically a trimmed reaction line or query.
 * @return The first matching arrow's location and kind, or a default-constructed
 *         arrow_info (pos == std::string_view::npos) if none was found.
 */
arrow_info find_arrow(std::string_view t);

/**
 * @brief The result of extract_falloff_markers(): the cleaned side text plus
 * the collider named by the marker, if one was present.
 */
struct falloff_extraction {
  std::string cleaned;  ///< The side text with every "(+...)" marker removed.
  std::string
      collider;  ///< Inner name of the first marker ("M", "N2", ...); empty if none.
};

/**
 * @brief Extracts "(+...)" fall-off markers like (+M) or (+N2) from a reaction side.
 *
 * @details Other parenthesized text is kept verbatim (unusual species names
 * may contain parens). The '+' inside "(+M)" must not be treated as a
 * species separator, so these groups are removed before the side is split on
 * '+' -- but the collider they name is chemically meaningful (it marks a
 * pressure-dependent fall-off reaction), so it is returned rather than
 * discarded.
 *
 * @param side One reaction side, e.g. "H+O2(+M)".
 * @return The cleaned text and the extracted collider name; see
 *         falloff_extraction.
 */
falloff_extraction extract_falloff_markers(std::string_view side);

/**
 * @brief Parses a single Arrhenius parameter token as a double.
 *
 * @details Tolerates Fortran-style exponent markers ('D'/'d' in place of
 * 'E'/'e'), which appear in mechanism files produced by legacy Fortran
 * kinetics tooling (e.g. "1.2D+14").
 *
 * Uses std::strtod rather than std::from_chars for two reasons. First,
 * Apple's bundled libc++ (Xcode/AppleClang) still ships only the integral
 * overloads of from_chars -- the floating-point overload fails to compile on
 * macOS even though it works on libstdc++ and recent upstream libc++.
 * Second, from_chars rejects a leading '+', and mechanism files really do
 * contain tokens like "+1.04400000E+005". strtod has been portable since
 * C89, at the cost of reading through a null-terminated C string (hence a
 * copy of the token, needed anyway for the D/d substitution) and of
 * consulting the process-wide C locale for the decimal point -- harmless
 * here, since ckgrep never touches locale state.
 *
 * @param token One whitespace-delimited rate token, e.g. "1.2D+14".
 * @return The parsed value, or std::nullopt if the token is not a valid
 *         floating-point literal (after D/d normalization), has trailing
 *         junk, or is out of double's representable range.
 */
std::optional<double> parse_arrhenius_value(std::string_view token);

/**
 * @brief Extracts the trailing Arrhenius coefficients [A, n, Ea] from the
 * text after a reaction's arrow.
 *
 * @details A CHEMKIN reaction line ends with exactly three whitespace-
 * delimited rate tokens (A, n, Ea). Splitting on whitespace and parsing the
 * last three tokens as whole units sidesteps the ambiguity that makes
 * character-wise classification fragile: a '+' inside a scientific-notation
 * exponent ("6.1400e+05") is indistinguishable from a species-joining '+'
 * when scanning one character at a time, but a whole token either parses as
 * a number or it does not.
 *
 * @param rest The text after the direction arrow (products + rate tail).
 * @return [A, n, Ea] if the last three whitespace-delimited tokens of
 *         @p rest all parse as doubles (see parse_arrhenius_value());
 *         std::nullopt if there are fewer than three tokens or any of the
 *         three fails to parse.
 */
std::optional<std::array<double, 3>> parse_arrhenius(std::string_view rest);

/**
 * @brief Splits one reaction side into species entries plus its third-body
 * classification.
 *
 * @details Extracts fall-off markers (see extract_falloff_markers()), splits
 * the remainder on '+', and parses each token's leading stoichiometric
 * coefficient (see utils::parse_coefficient()). Each entry's
 * parsed_species::expanded list is filled here, once, per the rule in
 * utils::expansion_count(). The bare third-body marker ("M"/"m") is not a
 * species: it sets parsed_side::third_body to mixture instead of producing
 * an entry, just as a "(+...)" marker sets it to falloff. Wildcard tokens
 * from queries (e.g. "*OH") pass through verbatim as species names.
 *
 * @param side One reaction side, e.g. "2H+O2(+M)".
 * @return The species entries in left-to-right order plus the third-body
 *         marker found, if any; see parsed_side.
 */
parsed_side parse_reaction_side(std::string_view side);

/**
 * @brief Parses one CHEMKIN-formatted reaction line.
 *
 * @details A trailing '!' comment, if any, is stripped first. The line must
 * contain a direction arrow (see find_arrow()). Handles order-independent
 * sides, stoichiometric coefficients (2CH3, 2 CH3, 0.5O2), third-body
 * markers on either side, and the trailing [A, n, Ea] rate tail (see
 * parse_arrhenius()): when the last three whitespace-delimited tokens after
 * the arrow all parse as numbers, the products end where they begin;
 * otherwise the whole remainder is product text and
 * parsed_reaction::arrhenius stays empty. A missing or malformed rate tail
 * is therefore not an error -- the species sides are still returned, so one
 * odd line never aborts a search.
 *
 * Query strings share this grammar minus the rate tail; parse_query()
 * composes find_arrow() and parse_reaction_side() directly rather than
 * calling this function, precisely so that a query like "O2 = O + O" cannot
 * have its trailing species mistaken for rate coefficients.
 *
 * @param line One physical line, not yet trimmed.
 * @return The parsed reaction (line_number left 0 for the caller to stamp),
 *         or std::nullopt if @p line is empty, has no arrow, or has no
 *         species on either side.
 */
std::optional<parsed_reaction> parse_reaction_line(std::string_view line);
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
