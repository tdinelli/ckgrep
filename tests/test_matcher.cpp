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

#include <vector>

#include "matcher.hpp"
#include "query.hpp"
#include "reaction_parser.hpp"
#include "string_utils.hpp"

namespace ckgrep {
namespace {
// Builds a parsed_species with its expanded occurrence list filled in, matching
// what parse_reaction_side() would produce for the same name and coefficient.
parsed_species sp(const std::string& name, double coefficient) {
  parsed_species result;
  result.name = name;
  result.coefficient = coefficient;
  result.expanded.assign(utils::expansion_count(coefficient), name);
  return result;
}

std::vector<parsed_species> side(std::vector<parsed_species> species) {
  return species;
}
}  // namespace

TEST(FlattenSpecies, IntegerCoefficientExpandsToRepeatedEntries) {
  std::vector<parsed_species> s = side({sp("H", 2.0)});
  EXPECT_EQ(flatten_species(s), (std::vector<std::string>{"H", "H"}));
}

TEST(FlattenSpecies, DefaultCoefficientExpandsToSingleEntry) {
  std::vector<parsed_species> s = side({sp("OH", 1.0)});
  EXPECT_EQ(flatten_species(s), (std::vector<std::string>{"OH"}));
}

TEST(FlattenSpecies, FractionalCoefficientStaysSingleEntry) {
  std::vector<parsed_species> s = side({sp("O2", 0.5)});
  EXPECT_EQ(flatten_species(s), (std::vector<std::string>{"O2"}));
}

TEST(FlattenSpecies, MultipleSpeciesExpandIndependently) {
  std::vector<parsed_species> s = side({sp("H", 2.0), sp("O2", 1.0)});
  EXPECT_EQ(flatten_species(s), (std::vector<std::string>{"H", "H", "O2"}));
}

TEST(FlattenSpecies, EmptySideExpandsToEmpty) {
  EXPECT_TRUE(flatten_species({}).empty());
}

TEST(CanAssign, EachTokenGetsADistinctSpecies) {
  query q = parse_query("H+O2");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_TRUE(can_assign(*q.any, {"H", "O2"}));
}

TEST(CanAssign, FailsWhenMoreTokensThanSpecies) {
  query q = parse_query("H+H+O2");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_FALSE(can_assign(*q.any, {"H", "O2"}));
}

TEST(CanAssign, RepeatedTokenRequiresRepeatedSpecies) {
  query q = parse_query("H+H");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_FALSE(can_assign(*q.any, {"H", "O2"}));
  EXPECT_TRUE(can_assign(*q.any, {"H", "H"}));
}

TEST(CanAssign, RequiresAugmentingPathToReassignContestedSpecies) {
  // "*" matches every species ("H", "O2", "N2"); "H" matches only "H". A
  // greedy assignment that lets "*" claim "H" first (processed before "H"
  // itself) leaves no species for "H" unless it backtracks and reassigns "*"
  // onto a different species. Kuhn's algorithm does that reassignment.
  query q = parse_query("*+H");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_TRUE(can_assign(*q.any, {"H", "O2"}));

  q = parse_query("O2+*+H");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_TRUE(can_assign(*q.any, {"H", "O2", "CH3"}));
}

TEST(CanAssign, NoMatchingSpeciesFails) {
  query q = parse_query("CH4");
  ASSERT_TRUE(q.any.has_value());
  EXPECT_FALSE(can_assign(*q.any, {"H", "O2"}));
}

TEST(CanAssign, EmptySideTriviallySucceeds) {
  query_side empty_side;
  EXPECT_TRUE(can_assign(empty_side, {"H", "O2"}));
}

TEST(SideMatches, ContainsModeAllowsExtraSpecies) {
  query q = parse_query("H");
  ASSERT_TRUE(q.any.has_value());
  std::vector<parsed_species> s = side({sp("H", 1.0), sp("O2", 1.0)});
  EXPECT_TRUE(side_matches(*q.any, s, match_mode::contains));
}

TEST(SideMatches, ExactModeRejectsExtraSpecies) {
  query q = parse_query("H");
  ASSERT_TRUE(q.any.has_value());
  std::vector<parsed_species> s = side({sp("H", 1.0), sp("O2", 1.0)});
  EXPECT_FALSE(side_matches(*q.any, s, match_mode::exact));
}

TEST(SideMatches, ExactModeAcceptsWhenCountsLineUp) {
  query q = parse_query("H+O2");
  ASSERT_TRUE(q.any.has_value());
  std::vector<parsed_species> s = side({sp("H", 1.0), sp("O2", 1.0)});
  EXPECT_TRUE(side_matches(*q.any, s, match_mode::exact));
}

TEST(SideMatches, ExactModeHonorsCoefficientExpansion) {
  query q = parse_query("H+H");
  ASSERT_TRUE(q.any.has_value());
  std::vector<parsed_species> s = side({sp("H", 2.0)});
  EXPECT_TRUE(side_matches(*q.any, s, match_mode::exact));
}

TEST(SideMatches, UnmatchedTokenFails) {
  query q = parse_query("CH4");
  ASSERT_TRUE(q.any.has_value());
  std::vector<parsed_species> s = side({sp("H", 1.0), sp("O2", 1.0)});
  EXPECT_FALSE(side_matches(*q.any, s, match_mode::contains));
}

TEST(Matches, AnyQueryMatchesReactantsSide) {
  query q = parse_query("CH4");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnyQueryMatchesProductsSide) {
  query q = parse_query("CO2");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnyQueryFailsWhenOnNeitherSide) {
  query q = parse_query("N2");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_FALSE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnchoredQueryRequiresMatchOnSpecifiedSide) {
  query q = parse_query("CH4=>CO2");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnchoredQueryFailsWhenSideIsSwapped) {
  query q = parse_query("CO2=>CH4");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_FALSE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnchoredQueryWithOnlyOneSideLeavesOtherUnconstrained) {
  query q = parse_query("CH4=>");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("N2", 1.0)});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, ExactModeOnBothAnchoredSides) {
  query q = parse_query("CH4+O2=>CO2+H2O");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_TRUE(matches(q, r, match_mode::exact));
}

TEST(Matches, ExactModeFailsWithLeftoverReactionSpecies) {
  query q = parse_query("CH4=>CO2+H2O");
  parsed_reaction r;
  r.reactants = side({sp("CH4", 1.0), sp("O2", 1.0)});
  r.products = side({sp("CO2", 1.0), sp("H2O", 1.0)});
  EXPECT_FALSE(matches(q, r, match_mode::exact));
}

TEST(Matches, GlobQueryMatchesViaWildcard) {
  query q = parse_query("*OH");
  parsed_reaction r;
  r.reactants = side({sp("CH3OH", 1.0)});
  r.products = side({sp("CH4", 1.0)});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(ThirdBodyMatches, ContainsModeIgnoresThirdBodyWhenUnconstrained) {
  query q = parse_query("H");
  parsed_reaction r;
  r.reactants = side({sp("H", 1.0)});
  r.third_body = third_body_kind::falloff;
  r.collider = "M";
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(ThirdBodyMatches, ExactModeRequiresNoThirdBodyWhenUnconstrained) {
  // Exact means exact: a query without a marker describes a reaction
  // without a third body.
  query q = parse_query("H");
  parsed_reaction r;
  r.reactants = side({sp("H", 1.0)});
  r.third_body = third_body_kind::falloff;
  r.collider = "M";
  EXPECT_FALSE(matches(q, r, match_mode::exact));
}

TEST(ThirdBodyMatches, FalloffConstraintRejectsPlainReaction) {
  query q = parse_query("H+O2(+M)=");
  parsed_reaction r;
  r.reactants = side({sp("H", 1.0), sp("O2", 1.0)});
  r.products = side({sp("HO2", 1.0)});
  EXPECT_FALSE(matches(q, r, match_mode::contains));
}

TEST(ThirdBodyMatches, GenericFalloffQueryMatchesSpecificCollider) {
  // "(+M)" in a query means any fall-off collider, per chemical convention.
  query q = parse_query("H+O2(+M)=");
  parsed_reaction r;
  r.reactants = side({sp("H", 1.0), sp("O2", 1.0)});
  r.products = side({sp("HO2", 1.0)});
  r.third_body = third_body_kind::falloff;
  r.collider = "N2";
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(ThirdBodyMatches, SpecificColliderQueryMustMatchReactionCollider) {
  query q = parse_query("(+N2)");
  parsed_reaction r;
  r.reactants = side({sp("H", 1.0)});
  r.third_body = third_body_kind::falloff;
  r.collider = "N2";
  EXPECT_TRUE(matches(q, r, match_mode::contains));
  r.collider = "AR";
  EXPECT_FALSE(matches(q, r, match_mode::contains));
}

TEST(ThirdBodyMatches, MixtureConstraintDistinguishesKinds) {
  query q = parse_query("M");
  parsed_reaction r;
  r.reactants = side({sp("H2", 1.0)});
  r.third_body = third_body_kind::mixture;
  r.collider = "M";
  EXPECT_TRUE(matches(q, r, match_mode::contains));
  r.third_body = third_body_kind::falloff;
  EXPECT_FALSE(matches(q, r, match_mode::contains));
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
