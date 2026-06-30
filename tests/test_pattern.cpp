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

#include "pattern.hpp"

namespace ckgrep {
TEST(MakePattern, LiteralTokenMatchesExactSpecies) {
  auto p = make_pattern("CH4");
  EXPECT_TRUE(p->matches("CH4"));
  EXPECT_FALSE(p->matches("CH3"));
}

TEST(MakePattern, SourceReturnsOriginalTokenTextCasePreserved) {
  auto p = make_pattern("ch4");
  EXPECT_EQ(p->source(), "ch4");
}

TEST(MakePattern, GlobWildcardTokenMatchesViaGlob) {
  auto p = make_pattern("C*");
  EXPECT_TRUE(p->matches("CH4"));
  EXPECT_TRUE(p->matches("C2H6"));
  EXPECT_FALSE(p->matches("O2"));
}

TEST(MakePattern, DefaultIsCaseInsensitive) {
  auto p = make_pattern("ch4");
  EXPECT_TRUE(p->matches("CH4"));
  EXPECT_TRUE(p->matches("Ch4"));
}

TEST(MakePattern, CaseSensitiveFlagRequiresExactCase) {
  auto p = make_pattern("ch4", /*case_sensitive=*/true);
  EXPECT_TRUE(p->matches("ch4"));
  EXPECT_FALSE(p->matches("CH4"));
}

TEST(GlobPattern, IsLiteralTrueWhenNoWildcards) {
  glob_pattern p("O2");
  EXPECT_TRUE(p.is_literal());
}

TEST(GlobPattern, IsLiteralFalseWithStarWildcard) {
  glob_pattern p("C*");
  EXPECT_FALSE(p.is_literal());
}

TEST(GlobPattern, IsLiteralFalseWithQuestionMarkWildcard) {
  glob_pattern p("C?4");
  EXPECT_FALSE(p.is_literal());
}

TEST(GlobPattern, QuestionMarkMatchesExactlyOneCharacter) {
  glob_pattern p("C?4");
  EXPECT_TRUE(p.matches("CH4"));
  EXPECT_TRUE(p.matches("C44"));  // '?' matches the middle '4' too
  EXPECT_FALSE(p.matches("C4"));  // not enough characters for '?'
}

TEST(GlobPattern, StarMatchesAnywhereInToken) {
  glob_pattern p("*OH");
  EXPECT_TRUE(p.matches("CH3OH"));
  EXPECT_TRUE(p.matches("OH"));
  EXPECT_FALSE(p.matches("CH4"));
}

TEST(GlobPattern, CaseInsensitiveLiteralMatch) {
  glob_pattern p("CH4", /*case_sensitive=*/false);
  EXPECT_TRUE(p.matches("ch4"));
  EXPECT_TRUE(p.matches("Ch4"));
}

TEST(GlobPattern, CaseSensitiveLiteralMatch) {
  glob_pattern p("CH4", /*case_sensitive=*/true);
  EXPECT_TRUE(p.matches("CH4"));
  EXPECT_FALSE(p.matches("ch4"));
}

TEST(GlobPattern, CaseSensitiveGlobMatch) {
  glob_pattern p("C*", /*case_sensitive=*/true);
  EXPECT_TRUE(p.matches("CH4"));
  EXPECT_FALSE(p.matches("ch4"));
}

TEST(GlobPattern, EmptySourceIsLiteralAndMatchesOnlyEmptySpecies) {
  glob_pattern p("");
  EXPECT_TRUE(p.is_literal());
  EXPECT_TRUE(p.matches(""));
  EXPECT_FALSE(p.matches("CH4"));
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
