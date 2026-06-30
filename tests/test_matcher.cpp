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
#include "reaction_scanner.hpp"

namespace ckgrep {
namespace {
std::vector<species_amount> side(std::vector<species_amount> species) {
  return species;
}
}  // namespace

TEST(ExpandSpecies, IntegerCoefficientExpandsToRepeatedEntries) {
  std::vector<species_amount> s = side({{"H", 2.0}});
  std::vector<std::string> expanded = expand_species(s);
  EXPECT_EQ(expanded, (std::vector<std::string>{"H", "H"}));
}

TEST(ExpandSpecies, DefaultCoefficientExpandsToSingleEntry) {
  std::vector<species_amount> s = side({{"OH", 1.0}});
  EXPECT_EQ(expand_species(s), (std::vector<std::string>{"OH"}));
}

TEST(ExpandSpecies, FractionalCoefficientStaysSingleEntry) {
  std::vector<species_amount> s = side({{"O2", 0.5}});
  EXPECT_EQ(expand_species(s), (std::vector<std::string>{"O2"}));
}

TEST(ExpandSpecies, MultipleSpeciesExpandIndependently) {
  std::vector<species_amount> s = side({{"H", 2.0}, {"O2", 1.0}});
  EXPECT_EQ(expand_species(s), (std::vector<std::string>{"H", "H", "O2"}));
}

TEST(ExpandSpecies, EmptySideExpandsToEmpty) {
  EXPECT_TRUE(expand_species({}).empty());
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
  std::vector<species_amount> s = side({{"H", 1.0}, {"O2", 1.0}});
  EXPECT_TRUE(side_matches(*q.any, s, match_mode::contains));
}

TEST(SideMatches, ExactModeRejectsExtraSpecies) {
  query q = parse_query("H");
  ASSERT_TRUE(q.any.has_value());
  std::vector<species_amount> s = side({{"H", 1.0}, {"O2", 1.0}});
  EXPECT_FALSE(side_matches(*q.any, s, match_mode::exact));
}

TEST(SideMatches, ExactModeAcceptsWhenCountsLineUp) {
  query q = parse_query("H+O2");
  ASSERT_TRUE(q.any.has_value());
  std::vector<species_amount> s = side({{"H", 1.0}, {"O2", 1.0}});
  EXPECT_TRUE(side_matches(*q.any, s, match_mode::exact));
}

TEST(SideMatches, ExactModeHonorsCoefficientExpansion) {
  query q = parse_query("H+H");
  ASSERT_TRUE(q.any.has_value());
  std::vector<species_amount> s = side({{"H", 2.0}});
  EXPECT_TRUE(side_matches(*q.any, s, match_mode::exact));
}

TEST(SideMatches, UnmatchedTokenFails) {
  query q = parse_query("CH4");
  ASSERT_TRUE(q.any.has_value());
  std::vector<species_amount> s = side({{"H", 1.0}, {"O2", 1.0}});
  EXPECT_FALSE(side_matches(*q.any, s, match_mode::contains));
}

TEST(Matches, AnyQueryMatchesReactantsSide) {
  query q = parse_query("CH4");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnyQueryMatchesProductsSide) {
  query q = parse_query("CO2");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnyQueryFailsWhenOnNeitherSide) {
  query q = parse_query("N2");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_FALSE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnchoredQueryRequiresMatchOnSpecifiedSide) {
  query q = parse_query("CH4=>CO2");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnchoredQueryFailsWhenSideIsSwapped) {
  query q = parse_query("CO2=>CH4");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_FALSE(matches(q, r, match_mode::contains));
}

TEST(Matches, AnchoredQueryWithOnlyOneSideLeavesOtherUnconstrained) {
  query q = parse_query("CH4=>");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"N2", 1.0}});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
}

TEST(Matches, ExactModeOnBothAnchoredSides) {
  query q = parse_query("CH4+O2=>CO2+H2O");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_TRUE(matches(q, r, match_mode::exact));
}

TEST(Matches, ExactModeFailsWithLeftoverReactionSpecies) {
  query q = parse_query("CH4=>CO2+H2O");
  reaction r;
  r.reactants = side({{"CH4", 1.0}, {"O2", 1.0}});
  r.products = side({{"CO2", 1.0}, {"H2O", 1.0}});
  EXPECT_FALSE(matches(q, r, match_mode::exact));
}

TEST(Matches, GlobQueryMatchesViaWildcard) {
  query q = parse_query("*OH");
  reaction r;
  r.reactants = side({{"CH3OH", 1.0}});
  r.products = side({{"CH4", 1.0}});
  EXPECT_TRUE(matches(q, r, match_mode::contains));
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
