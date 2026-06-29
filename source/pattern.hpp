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
#include <string>
#include <string_view>

namespace ckgrep {
/**
 * @brief Abstract interface for a single query token tested against species names.
 *
 * @details Represents one matchable token from a user query and answers the only
 * question the matching engine ever asks of it: does it match a given species
 * name? The concrete matching strategy -- glob, literal, or (in future) regex --
 * is hidden behind this interface. The rest of the engine holds patterns through
 * a species_pattern pointer and calls matches() polymorphically, never knowing or
 * caring which subclass it has.
 *
 * This is the extension seam: adding a new matching kind means writing one new
 * subclass and teaching make_pattern() to build it. No calling code changes.
 *
 * @note Instances are created via make_pattern() rather than constructed directly.
 *       The virtual destructor ensures correct cleanup when a derived object is
 *       deleted through a species_pattern pointer.
 */
class species_pattern {
 public:
  virtual ~species_pattern() = default;

  /**
   * @brief Tests whether this pattern matches a species name.
   * @param species The species name to test.
   * @return true if the pattern matches @p species, false otherwise.
   */
  [[nodiscard]] virtual bool matches(std::string_view species) const = 0;

  /**
   * @brief Returns the original query text for this token.
   *
   * @details The text is preserved exactly as the user wrote it (case included)
   * so it can be echoed verbatim in output and diagnostic messages, independent
   * of any normalization applied internally for matching.
   *
   * @return A const reference to the token's original source text.
   */
  [[nodiscard]] virtual const std::string& source() const = 0;
};

/**
 * @brief A glob-style species pattern supporting '*' and '?' wildcards.
 *
 * @details Implements species_pattern using shell-style globbing: '*' matches any
 * run of characters (including none) and '?' matches exactly one character. A
 * token containing no wildcards is handled as a degenerate glob that reduces to a
 * plain equality check, so no separate literal-pattern class is needed; see
 * is_literal().
 *
 * Matching is case-insensitive by default, since CHEMKIN species are conventionally
 * uppercase but real-world files vary in casing. Pass case_sensitive = true at
 * construction to require an exact-case match instead.
 *
 * @code
 * glob_pattern p("C*");            // case-insensitive by default
 * p.matches("CH4");                // -> true
 * p.matches("ch4");                // -> true  (case-insensitive)
 * p.matches("O2");                 // -> false
 *
 * glob_pattern lit("O2");          // no wildcards
 * lit.is_literal();                // -> true
 * @endcode
 */
class glob_pattern : public species_pattern {
 public:
  /**
   * @brief Constructs a glob pattern from a query token.
   *
   * @details Precomputes a normalized form of the token (lowercased unless
   * @p case_sensitive is true) and whether the token is wildcard-free, so that
   * matches() performs no redundant per-call setup.
   *
   * @param source         The original query token; ownership is taken by move.
   * @param case_sensitive When true, matching requires exact case. Defaults to
   *                       false (case-insensitive).
   */
  explicit glob_pattern(std::string source, bool case_sensitive = false);

  /**
   * @brief Tests whether this glob matches a species name.
   *
   * @details Honors the case sensitivity chosen at construction. Wildcard-free
   * patterns short-circuit to a direct comparison; otherwise the glob matcher is
   * run against the normalized forms.
   *
   * @param species The species name to test.
   * @return true if the glob matches the entirety of @p species, false otherwise.
   */
  [[nodiscard]] bool matches(std::string_view species) const override;

  /**
   * @brief Returns the original token text, case preserved.
   * @return A const reference to the unmodified source token.
   */
  [[nodiscard]] const std::string& source() const override {
    return source_;
  }

  /**
   * @brief Reports whether the pattern contains no wildcards.
   *
   * @details A literal pattern (no '*' or '?') matches via plain equality rather
   * than the glob algorithm. Computed once at construction.
   *
   * @return true if the token is a plain species name with no wildcards.
   */
  [[nodiscard]] bool is_literal() const {
    return is_literal_;
  }

 private:
  std::string source_;      ///< Original token text; case preserved for output.
  std::string normalized_;  ///< Lowercased form when case-insensitive; else == source_.
  bool case_sensitive_;     ///< Whether matching requires an exact-case match.
  bool is_literal_;         ///< True when the token contains no '*' or '?'.
};

/**
 * @brief Factory that builds the appropriate species_pattern for a query token.
 *
 * @details Centralizes the choice of concrete pattern kind so that callers depend
 * only on the species_pattern interface. Today every token becomes a glob_pattern;
 * the @p use_regex flag is a reserved hook, wired through the signature now so that
 * callers already pass it, ready for a future regex_pattern subclass to be selected
 * here without changing any call sites.
 *
 * @param source         The query token; ownership is taken by move.
 * @param case_sensitive When true, matching requires exact case. Defaults to false.
 * @param use_regex      Reserved for future regex support; currently ignored.
 * @return An owning pointer to a species_pattern for @p source.
 */
std::unique_ptr<species_pattern>
make_pattern(std::string source, bool case_sensitive = false, bool use_regex = false);

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
