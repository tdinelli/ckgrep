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

#include "query.hpp"

namespace ckgrep {
TEST(ParseQuery, NoArrowPopulatesAny) {
  query q = parse_query("CH4 + O2");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_FALSE(q.reactants.has_value());
  EXPECT_FALSE(q.products.has_value());
  ASSERT_EQ(q.any->tokens.size(), 2U);
  EXPECT_EQ(q.any->tokens[0]->source(), "CH4");
  EXPECT_EQ(q.any->tokens[1]->source(), "O2");
}

TEST(ParseQuery, ArrowPopulatesReactantsAndProducts) {
  query q = parse_query("CH4 => *OH");
  ASSERT_TRUE(q.reactants.has_value());
  ASSERT_TRUE(q.products.has_value());
  EXPECT_FALSE(q.any.has_value());
  ASSERT_EQ(q.reactants->tokens.size(), 1U);
  EXPECT_EQ(q.reactants->tokens[0]->source(), "CH4");
  ASSERT_EQ(q.products->tokens.size(), 1U);
  EXPECT_EQ(q.products->tokens[0]->source(), "*OH");
}

TEST(ParseQuery, ReversibleArrowIsRecognized) {
  query q = parse_query("O2 <=> O + O");
  ASSERT_TRUE(q.reactants.has_value());
  ASSERT_TRUE(q.products.has_value());
  ASSERT_EQ(q.reactants->tokens.size(), 1U);
  EXPECT_EQ(q.reactants->tokens[0]->source(), "O2");
  // "O + O" parses as two separate tokens.
  ASSERT_EQ(q.products->tokens.size(), 2U);
  EXPECT_EQ(q.products->tokens[0]->source(), "O");
  EXPECT_EQ(q.products->tokens[1]->source(), "O");
}

TEST(ParseQuery, LongestArrowFormWinsOverShorterSubstrings) {
  // "<=>" contains "=>" and "=" as substrings; the longest form must be
  // tried first so it isn't mistaken for a shorter arrow.
  query q = parse_query("A <=> B");
  ASSERT_TRUE(q.reactants.has_value());
  ASSERT_TRUE(q.products.has_value());
  EXPECT_EQ(q.reactants->tokens[0]->source(), "A");
  EXPECT_EQ(q.products->tokens[0]->source(), "B");
}

TEST(ParseQuery, IrreversibleArrowIsRecognized) {
  query q = parse_query("A => B");
  ASSERT_TRUE(q.reactants.has_value());
  ASSERT_TRUE(q.products.has_value());
  EXPECT_EQ(q.reactants->tokens[0]->source(), "A");
  EXPECT_EQ(q.products->tokens[0]->source(), "B");
}

TEST(ParseQuery, PlainEqualsArrowIsRecognized) {
  query q = parse_query("A=B");
  ASSERT_TRUE(q.reactants.has_value());
  ASSERT_TRUE(q.products.has_value());
  EXPECT_EQ(q.reactants->tokens[0]->source(), "A");
  EXPECT_EQ(q.products->tokens[0]->source(), "B");
}

TEST(ParseQuery, BlankLeftSideLeavesReactantsUnset) {
  // An arrow with nothing meaningful to its left (e.g. user only cares about
  // products) leaves reactants unset rather than populated-but-empty.
  query q = parse_query("=> H2O");
  EXPECT_FALSE(q.reactants.has_value());
  ASSERT_TRUE(q.products.has_value());
  EXPECT_EQ(q.products->tokens[0]->source(), "H2O");
}

TEST(ParseQuery, BlankRightSideLeavesProductsUnset) {
  query q = parse_query("CH4 =>");
  ASSERT_TRUE(q.reactants.has_value());
  EXPECT_FALSE(q.products.has_value());
  EXPECT_EQ(q.reactants->tokens[0]->source(), "CH4");
}

TEST(ParseQuery, IntegerCoefficientExpandsIntoRepeatedTokens) {
  query q = parse_query("2H");
  ASSERT_TRUE(q.any.has_value());
  ASSERT_EQ(q.any->tokens.size(), 2U);
  EXPECT_EQ(q.any->tokens[0]->source(), "H");
  EXPECT_EQ(q.any->tokens[1]->source(), "H");
}

TEST(ParseQuery, CoefficientExpansionMatchesExplicitPlusForm) {
  query expanded = parse_query("2H");
  query explicit_form = parse_query("H+H");
  ASSERT_TRUE(expanded.any.has_value());
  ASSERT_TRUE(explicit_form.any.has_value());
  EXPECT_EQ(expanded.any->tokens.size(), explicit_form.any->tokens.size());
}

TEST(ParseQuery, FractionalCoefficientStaysSingleToken) {
  query q = parse_query("0.5O2");
  ASSERT_TRUE(q.any.has_value());
  ASSERT_EQ(q.any->tokens.size(), 1U);
  EXPECT_EQ(q.any->tokens[0]->source(), "O2");
}

TEST(ParseQuery, WhitespaceAroundTokensIsTrimmed) {
  query q = parse_query("  CH4  +  O2  ");
  ASSERT_TRUE(q.any.has_value());
  ASSERT_EQ(q.any->tokens.size(), 2U);
  EXPECT_EQ(q.any->tokens[0]->source(), "CH4");
  EXPECT_EQ(q.any->tokens[1]->source(), "O2");
}

TEST(ParseQuery, GlobWildcardTokenIsPreservedAndMatches) {
  query q = parse_query("*OH");
  ASSERT_TRUE(q.any.has_value());
  ASSERT_EQ(q.any->tokens.size(), 1U);
  EXPECT_TRUE(q.any->tokens[0]->matches("CH3OH"));
  EXPECT_FALSE(q.any->tokens[0]->matches("CH4"));
}

TEST(ParseQuery, DefaultIsCaseInsensitive) {
  query q = parse_query("ch4");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_TRUE(q.any->tokens[0]->matches("CH4"));
}

TEST(ParseQuery, CaseSensitiveFlagRequiresExactCase) {
  query q = parse_query("ch4", /*case_sensitive=*/true);
  ASSERT_TRUE(q.any.has_value());
  EXPECT_FALSE(q.any->tokens[0]->matches("CH4"));
  EXPECT_TRUE(q.any->tokens[0]->matches("ch4"));
}

TEST(ParseQuery, EmptyQueryThrows) {
  EXPECT_THROW(parse_query(""), std::runtime_error);
}

TEST(ParseQuery, WhitespaceOnlyQueryThrows) {
  EXPECT_THROW(parse_query("   "), std::runtime_error);
}

TEST(ParseQuery, ArrowWithNoSpeciesOnEitherSideThrows) {
  EXPECT_THROW(parse_query("=>"), std::runtime_error);
  EXPECT_THROW(parse_query("="), std::runtime_error);
}

TEST(ParseQuery, NoMarkerLeavesThirdBodyUnset) {
  query q = parse_query("CH4=CO2");
  EXPECT_FALSE(q.third_body.has_value());
}

TEST(ParseQuery, FalloffMarkerBecomesAnyColliderConstraint) {
  query q = parse_query("H+O2(+M)=");
  ASSERT_TRUE(q.third_body.has_value());
  EXPECT_EQ(q.third_body->kind, third_body_kind::falloff);
  EXPECT_TRUE(q.third_body->any_collider);
  // The marker is a constraint, not a species token.
  ASSERT_TRUE(q.reactants.has_value());
  EXPECT_EQ(q.reactants->tokens.size(), 2U);
}

TEST(ParseQuery, SpecificColliderBecomesPattern) {
  query q = parse_query("H+OH(+N2)=");
  ASSERT_TRUE(q.third_body.has_value());
  EXPECT_EQ(q.third_body->kind, third_body_kind::falloff);
  EXPECT_FALSE(q.third_body->any_collider);
  ASSERT_NE(q.third_body->collider, nullptr);
  EXPECT_TRUE(q.third_body->collider->matches("N2"));
  EXPECT_FALSE(q.third_body->collider->matches("AR"));
}

TEST(ParseQuery, BareMBecomesMixtureConstraint) {
  query q = parse_query("H+O2+M=HO2+M");
  ASSERT_TRUE(q.third_body.has_value());
  EXPECT_EQ(q.third_body->kind, third_body_kind::mixture);
  ASSERT_TRUE(q.reactants.has_value());
  EXPECT_EQ(q.reactants->tokens.size(), 2U);  // M is not a token
}

TEST(ParseQuery, ConstraintOnlyQueryIsValid) {
  // "(+M)" alone: every fall-off reaction, no species constrained.
  query q = parse_query("(+M)");
  EXPECT_FALSE(q.any.has_value());
  EXPECT_FALSE(q.reactants.has_value());
  EXPECT_FALSE(q.products.has_value());
  ASSERT_TRUE(q.third_body.has_value());
  EXPECT_EQ(q.third_body->kind, third_body_kind::falloff);
  EXPECT_TRUE(q.third_body->any_collider);
}

TEST(ParseQuery, ArrowlessBareMIsMixtureConstraintOnly) {
  query q = parse_query("M");
  EXPECT_FALSE(q.any.has_value());
  ASSERT_TRUE(q.third_body.has_value());
  EXPECT_EQ(q.third_body->kind, third_body_kind::mixture);
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
