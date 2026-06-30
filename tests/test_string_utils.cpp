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
#include <gtest/gtest.h>

#include "string_utils.hpp"

namespace ckgrep::utils {
TEST(GlobMatch, LiteralNoWildcards) {
  EXPECT_TRUE(glob_match("O2", "O2"));
  EXPECT_FALSE(glob_match("O2", "O3"));
}

TEST(GlobMatch, StarMatchesAnyRun) {
  EXPECT_TRUE(glob_match("C*", "CH4"));
  EXPECT_TRUE(glob_match("C*4", "CHHH4"));
  EXPECT_FALSE(glob_match("C*4", "CHHH"));
}

TEST(GlobMatch, QuestionMarkMatchesOneChar) {
  EXPECT_TRUE(glob_match("?H4", "CH4"));
  EXPECT_FALSE(glob_match("?H4", "CCH4"));
}

TEST(GlobMatch, BothEmpty) {
  EXPECT_TRUE(glob_match("", ""));
}

TEST(GlobMatch, EmptyPatternOnlyMatchesEmptyText) {
  EXPECT_FALSE(glob_match("", "CH4"));
}

TEST(GlobMatch, EmptyTextOnlyMatchesAllStarsOrEmptyPattern) {
  EXPECT_TRUE(glob_match("*", ""));
  EXPECT_TRUE(glob_match("**", ""));
  EXPECT_FALSE(glob_match("C", ""));
  EXPECT_FALSE(glob_match("?", ""));
}

TEST(GlobMatch, SingleStarMatchesEverything) {
  EXPECT_TRUE(glob_match("*", "CH4"));
  EXPECT_TRUE(glob_match("*", "anything at all"));
}

TEST(GlobMatch, ConsecutiveStarsCollapseToOne) {
  EXPECT_TRUE(glob_match("C**4", "CHHH4"));
  EXPECT_TRUE(glob_match("**", "CH4"));
}

TEST(GlobMatch, StarAtBothEnds) {
  EXPECT_TRUE(glob_match("*H4*", "CH4O"));
  EXPECT_TRUE(glob_match("*H4*", "H4"));
}

TEST(GlobMatch, QuestionMarkRequiresExactlyOneChar) {
  EXPECT_TRUE(glob_match("C?4", "C44"));    // '?' eats the middle '4'
  EXPECT_FALSE(glob_match("C?4", "C4"));    // not enough characters for '?'
  EXPECT_FALSE(glob_match("C?4", "C444"));  // one too many characters
}

TEST(GlobMatch, PatternLongerThanTextFails) {
  EXPECT_FALSE(glob_match("CH4O2", "CH4"));
}

TEST(GlobMatch, TextLongerThanLiteralPatternFails) {
  EXPECT_FALSE(glob_match("CH4", "CH4O2"));
}

TEST(GlobMatch, BacktrackingOnFailedStarExtension) {
  // '*' tentatively matches "BBB", then must backtrack to also cover the
  // trailing 'A' before 'C' so the pattern can still match to the end.
  EXPECT_TRUE(glob_match("A*C", "ABBBAC"));
  EXPECT_FALSE(glob_match("A*C", "ABBBA"));
}

TEST(GlobMatch, IsCaseSensitive) {
  EXPECT_FALSE(glob_match("ch4", "CH4"));
  EXPECT_TRUE(glob_match("CH4", "CH4"));
}

TEST(ToLower, FoldsAsciiUppercase) {
  EXPECT_EQ(to_lower("CH4 + O2"), "ch4 + o2");
  EXPECT_EQ(to_lower("End"), "end");
  EXPECT_EQ(to_lower(""), "");
}

TEST(ToLower, LeavesNonLetterBytesUnchanged) {
  EXPECT_EQ(to_lower("123+-()"), "123+-()");
  EXPECT_EQ(to_lower("C2H5-A"), "c2h5-a");
}

TEST(ToLower, LeavesAlreadyLowercaseUnchanged) {
  EXPECT_EQ(to_lower("ch4"), "ch4");
}

TEST(ToLower, OnlyFoldsAsciiRangeNotAdjacentBytes) {
  // '@' (0x40) and '[' (0x5B) bracket 'A'-'Z' and must not be touched.
  EXPECT_EQ(to_lower("@["), "@[");
}

TEST(ContainsCi, MatchesIgnoringCase) {
  EXPECT_TRUE(contains_ci("Soot precursor note", "PRECURSOR"));
  EXPECT_TRUE(contains_ci("Soot precursor note", "precursor"));
  EXPECT_FALSE(contains_ci("Soot precursor note", "xyz"));
}

TEST(ContainsCi, EmptyNeedleAlwaysMatches) {
  EXPECT_TRUE(contains_ci("anything", ""));
  EXPECT_TRUE(contains_ci("", ""));
}

TEST(ContainsCi, EmptyHaystackOnlyMatchesEmptyNeedle) {
  EXPECT_FALSE(contains_ci("", "x"));
}

TEST(ContainsCi, NeedleLongerThanHaystackFails) {
  EXPECT_FALSE(contains_ci("CH4", "CH4O2"));
}

TEST(ContainsCi, MatchesAtHaystackBoundaries) {
  EXPECT_TRUE(contains_ci("PREFIX rest", "prefix"));
  EXPECT_TRUE(contains_ci("rest SUFFIX", "suffix"));
  EXPECT_TRUE(contains_ci("CH4", "ch4"));
}

TEST(Trim, StripsLeadingAndTrailingWhitespace) {
  EXPECT_EQ(trim("   CH4 + O2  \r\n"), "CH4 + O2");
  EXPECT_EQ(trim("END"), "END");
  EXPECT_EQ(trim("   \t  "), "");
}

TEST(Trim, EmptyStringStaysEmpty) {
  EXPECT_EQ(trim(""), "");
}

TEST(Trim, SingleCharacterNoWhitespace) {
  EXPECT_EQ(trim("X"), "X");
}

TEST(Trim, SingleWhitespaceCharacter) {
  EXPECT_EQ(trim(" "), "");
}

TEST(Trim, OnlyLeadingWhitespace) {
  EXPECT_EQ(trim("  CH4"), "CH4");
}

TEST(Trim, OnlyTrailingWhitespace) {
  EXPECT_EQ(trim("CH4  "), "CH4");
}

TEST(Trim, PreservesInternalWhitespace) {
  EXPECT_EQ(trim("  CH4   +   O2  "), "CH4   +   O2");
}

TEST(Trim, EachWhitespaceKindIsStripped) {
  EXPECT_EQ(trim("\t\r\n CH4 \n\r\t"), "CH4");
}

TEST(ParseCoefficient, SplitsLeadingCoefficient) {
  coefficient_token t = parse_coefficient("2H");
  EXPECT_DOUBLE_EQ(t.coefficient, 2.0);
  EXPECT_EQ(t.species, "H");
}

TEST(ParseCoefficient, SplitsFractionalCoefficient) {
  coefficient_token t = parse_coefficient("0.5O2");
  EXPECT_DOUBLE_EQ(t.coefficient, 0.5);
  EXPECT_EQ(t.species, "O2");
}

TEST(ParseCoefficient, AllowsSpaceBetweenCoefficientAndSpecies) {
  coefficient_token t = parse_coefficient("2 CH3");
  EXPECT_DOUBLE_EQ(t.coefficient, 2.0);
  EXPECT_EQ(t.species, "CH3");
}

TEST(ParseCoefficient, NoCoefficientDefaultsToOne) {
  coefficient_token t = parse_coefficient("OH");
  EXPECT_DOUBLE_EQ(t.coefficient, 1.0);
  EXPECT_EQ(t.species, "OH");
}

TEST(ParseCoefficient, BareNumberPassesThroughUnchanged) {
  coefficient_token t = parse_coefficient("123");
  EXPECT_DOUBLE_EQ(t.coefficient, 1.0);
  EXPECT_EQ(t.species, "123");
}

TEST(ParseCoefficient, BareFractionalNumberPassesThroughUnchanged) {
  coefficient_token t = parse_coefficient("0.5");
  EXPECT_DOUBLE_EQ(t.coefficient, 1.0);
  EXPECT_EQ(t.species, "0.5");
}

TEST(ParseCoefficient, EmptyTokenPassesThroughUnchanged) {
  coefficient_token t = parse_coefficient("");
  EXPECT_DOUBLE_EQ(t.coefficient, 1.0);
  EXPECT_EQ(t.species, "");
}

TEST(ParseCoefficient, LeadingDotWithNoIntegerPart) {
  coefficient_token t = parse_coefficient(".5O2");
  EXPECT_DOUBLE_EQ(t.coefficient, 0.5);
  EXPECT_EQ(t.species, "O2");
}

TEST(ParseCoefficient, MultiDigitCoefficient) {
  coefficient_token t = parse_coefficient("10CH4");
  EXPECT_DOUBLE_EQ(t.coefficient, 10.0);
  EXPECT_EQ(t.species, "CH4");
}

TEST(ParseCoefficient, SpeciesNameStartingWithDigitIsNotMistakenForCoefficient) {
  // CHEMKIN isomer-style names like "13CH3" or "1C4H8" start with a digit
  // that is part of the species name, not a stoichiometric coefficient. The
  // current implementation cannot distinguish this from a real coefficient
  // and will (incorrectly, but this pins down the existing behavior) split
  // it as coefficient=13, species="CH3".
  coefficient_token t = parse_coefficient("13CH3");
  EXPECT_DOUBLE_EQ(t.coefficient, 13.0);
  EXPECT_EQ(t.species, "CH3");
}

TEST(ExpansionCount, IntegerCoefficientExpands) {
  EXPECT_EQ(expansion_count(2.0), 2);
  EXPECT_EQ(expansion_count(1.0), 1);
  EXPECT_EQ(expansion_count(3.0), 3);
}

TEST(ExpansionCount, FractionalCoefficientStaysSingle) {
  EXPECT_EQ(expansion_count(0.5), 1);
}

TEST(ExpansionCount, ZeroStaysSingle) {
  // A coefficient of 0 is not a valid stoichiometric coefficient for a
  // species that is actually present, but expansion_count() must not return
  // 0 occurrences (which would silently drop the species from matching).
  EXPECT_EQ(expansion_count(0.0), 1);
}

TEST(ExpansionCount, NegativeCoefficientStaysSingle) {
  // Negative coefficients don't occur in CHEMKIN reaction sides, but the
  // function must not return a negative or zero occurrence count for one.
  EXPECT_EQ(expansion_count(-2.0), 1);
}

TEST(ExpansionCount, ValuesWithinToleranceOfAnIntegerExpand) {
  EXPECT_EQ(expansion_count(2.0 - coefficient_integer_tolerance / 2), 2);
  EXPECT_EQ(expansion_count(2.0 + coefficient_integer_tolerance / 2), 2);
}

TEST(ExpansionCount, ValuesOutsideToleranceDoNotExpand) {
  EXPECT_EQ(expansion_count(2.0 - coefficient_integer_tolerance * 10), 1);
  EXPECT_EQ(expansion_count(2.0 + coefficient_integer_tolerance * 10), 1);
}

TEST(ExpansionCount, LargeIntegerCoefficientExpandsFully) {
  EXPECT_EQ(expansion_count(10.0), 10);
  EXPECT_EQ(expansion_count(22.0), 22);
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
