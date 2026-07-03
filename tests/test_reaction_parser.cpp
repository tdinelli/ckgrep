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

#include "reaction_parser.hpp"

namespace ckgrep {
TEST(FindArrow, ReversibleLong) {
  auto a = find_arrow("H2+O2<=>H2O+O");
  EXPECT_EQ(a.pos, 5U);
  EXPECT_EQ(a.len, 3U);
  EXPECT_TRUE(a.reversible);
}

TEST(FindArrow, Irreversible) {
  auto a = find_arrow("H2+O2=>H2O+O");
  EXPECT_EQ(a.pos, 5U);
  EXPECT_EQ(a.len, 2U);
  EXPECT_FALSE(a.reversible);
}

TEST(FindArrow, PlainEquals) {
  auto a = find_arrow("A=B");
  EXPECT_EQ(a.pos, 1U);
  EXPECT_EQ(a.len, 1U);
  EXPECT_TRUE(a.reversible);
}

TEST(FindArrow, LongestFormWinsOverShorter) {
  // "<=" contains "="; "<=>" must win.
  auto a = find_arrow("A<=>B");
  EXPECT_EQ(a.len, 3U);
  EXPECT_TRUE(a.reversible);
}

TEST(FindArrow, NoArrow) {
  auto a = find_arrow("ELEMENTS");
  EXPECT_EQ(a.pos, std::string_view::npos);
}

TEST(ExtractFalloffMarkers, StripsParenPlusMAndRecordsCollider) {
  falloff_extraction e = extract_falloff_markers("H+O2(+M)");
  EXPECT_EQ(e.cleaned, "H+O2");
  EXPECT_EQ(e.collider, "M");
}

TEST(ExtractFalloffMarkers, StripsParenPlusSpeciesAndRecordsCollider) {
  falloff_extraction e = extract_falloff_markers("H+OH(+N2)");
  EXPECT_EQ(e.cleaned, "H+OH");
  EXPECT_EQ(e.collider, "N2");
}

TEST(ExtractFalloffMarkers, KeepsOtherParens) {
  // Unusual species name with parens not starting with '+' must be kept.
  falloff_extraction e = extract_falloff_markers("A(B)+C");
  EXPECT_EQ(e.cleaned, "A(B)+C");
  EXPECT_TRUE(e.collider.empty());
}

TEST(ExtractFalloffMarkers, NoMarkersUnchanged) {
  falloff_extraction e = extract_falloff_markers("H+O2+OH");
  EXPECT_EQ(e.cleaned, "H+O2+OH");
  EXPECT_TRUE(e.collider.empty());
}

TEST(ExtractFalloffMarkers, MultipleMarkersAllStripped) {
  falloff_extraction e = extract_falloff_markers("H(+M)+O2(+M)");
  EXPECT_EQ(e.cleaned, "H+O2");
  EXPECT_EQ(e.collider, "M");
}

TEST(ParseArrheniusValue, PlainAndScientificForms) {
  EXPECT_DOUBLE_EQ(parse_arrhenius_value("174").value(), 174.0);
  EXPECT_DOUBLE_EQ(parse_arrhenius_value("6.1400e+05").value(), 6.14e5);
}

TEST(ParseArrheniusValue, FortranExponentMarkers) {
  EXPECT_DOUBLE_EQ(parse_arrhenius_value("1.2D+14").value(), 1.2e14);
  EXPECT_DOUBLE_EQ(parse_arrhenius_value("1.0d-3").value(), 1.0e-3);
}

TEST(ParseArrheniusValue, LeadingPlusAccepted) {
  // from_chars would reject this; strtod must not.
  EXPECT_DOUBLE_EQ(parse_arrhenius_value("+1.04400000E+005").value(), 1.044e5);
}

TEST(ParseArrheniusValue, RejectsNonNumericTokens) {
  EXPECT_FALSE(parse_arrhenius_value("H2O").has_value());
  EXPECT_FALSE(parse_arrhenius_value("1.0x").has_value());
  EXPECT_FALSE(parse_arrhenius_value("").has_value());
}

TEST(ParseReactionSide, SimpleSingleSpecies) {
  parsed_side side = parse_reaction_side("OH");
  ASSERT_EQ(side.species.size(), 1U);
  EXPECT_EQ(side.species[0].name, "OH");
  EXPECT_DOUBLE_EQ(side.species[0].coefficient, 1.0);
  EXPECT_EQ(side.species[0].expanded, (std::vector<std::string>{"OH"}));
  EXPECT_EQ(side.third_body, third_body_kind::none);
}

TEST(ParseReactionSide, MultipleSpecies) {
  parsed_side side = parse_reaction_side("H+O2+OH");
  ASSERT_EQ(side.species.size(), 3U);
  EXPECT_EQ(side.species[0].name, "H");
  EXPECT_EQ(side.species[1].name, "O2");
  EXPECT_EQ(side.species[2].name, "OH");
}

TEST(ParseReactionSide, IntegerCoefficientFillsExpanded) {
  parsed_side side = parse_reaction_side("2H");
  ASSERT_EQ(side.species.size(), 1U);
  EXPECT_EQ(side.species[0].name, "H");
  EXPECT_DOUBLE_EQ(side.species[0].coefficient, 2.0);
  EXPECT_EQ(side.species[0].expanded, (std::vector<std::string>{"H", "H"}));
}

TEST(ParseReactionSide, FractionalCoefficientExpandsToOne) {
  parsed_side side = parse_reaction_side("0.5O2");
  ASSERT_EQ(side.species.size(), 1U);
  EXPECT_DOUBLE_EQ(side.species[0].coefficient, 0.5);
  EXPECT_EQ(side.species[0].expanded, (std::vector<std::string>{"O2"}));
}

TEST(ParseReactionSide, BareMBecomesMixtureThirdBody) {
  parsed_side side = parse_reaction_side("H+M+OH");
  ASSERT_EQ(side.species.size(), 2U);
  EXPECT_EQ(side.species[0].name, "H");
  EXPECT_EQ(side.species[1].name, "OH");
  EXPECT_EQ(side.third_body, third_body_kind::mixture);
  EXPECT_EQ(side.collider, "M");
}

TEST(ParseReactionSide, FalloffMarkerBecomesFalloffThirdBody) {
  parsed_side side = parse_reaction_side("H+O2(+M)");
  ASSERT_EQ(side.species.size(), 2U);
  EXPECT_EQ(side.species[0].name, "H");
  EXPECT_EQ(side.species[1].name, "O2");
  EXPECT_EQ(side.third_body, third_body_kind::falloff);
  EXPECT_EQ(side.collider, "M");
}

TEST(ParseReactionSide, SpecificColliderRecorded) {
  parsed_side side = parse_reaction_side("H2O(+N2)");
  ASSERT_EQ(side.species.size(), 1U);
  EXPECT_EQ(side.species[0].name, "H2O");
  EXPECT_EQ(side.third_body, third_body_kind::falloff);
  EXPECT_EQ(side.collider, "N2");
}

TEST(ParseReactionSide, SpaceBetweenCoefficientAndSpecies) {
  parsed_side side = parse_reaction_side("2 CH3");
  ASSERT_EQ(side.species.size(), 1U);
  EXPECT_EQ(side.species[0].name, "CH3");
  EXPECT_DOUBLE_EQ(side.species[0].coefficient, 2.0);
}

TEST(ParseReactionSide, EmptySideReturnsEmpty) {
  EXPECT_TRUE(parse_reaction_side("").species.empty());
  EXPECT_TRUE(parse_reaction_side("   ").species.empty());
}

TEST(ParseReactionSide, TwoHEquivalentTo2H) {
  parsed_side explicit_form = parse_reaction_side("H+H");
  parsed_side coeff_form = parse_reaction_side("2H");
  ASSERT_EQ(explicit_form.species.size(), 2U);
  ASSERT_EQ(coeff_form.species.size(), 1U);
  // Both yield the same expanded list.
  std::vector<std::string> exp_explicit;
  std::vector<std::string> exp_coeff;
  for (const auto& ps : explicit_form.species) {
    exp_explicit.insert(exp_explicit.end(), ps.expanded.begin(), ps.expanded.end());
  }
  for (const auto& ps : coeff_form.species) {
    exp_coeff.insert(exp_coeff.end(), ps.expanded.begin(), ps.expanded.end());
  }
  EXPECT_EQ(exp_explicit, exp_coeff);
}

TEST(ParseArrhenius, ParsesLastThreeTokens) {
  auto a = parse_arrhenius("H2O 1.0e+13 0.0 5000.0");
  ASSERT_TRUE(a.has_value());
  EXPECT_DOUBLE_EQ(a.value()[0], 1.0e+13);
  EXPECT_DOUBLE_EQ(a.value()[1], 0.0);
  EXPECT_DOUBLE_EQ(a.value()[2], 5000.0);
}

TEST(ParseArrhenius, NegativeAndExplicitPlusExponents) {
  auto a = parse_arrhenius("products 4.57700000E+019 -1.40000000E+000 +1.04400000E+005");
  ASSERT_TRUE(a.has_value());
  EXPECT_DOUBLE_EQ(a.value()[0], 4.577e19);
  EXPECT_DOUBLE_EQ(a.value()[1], -1.4);
  EXPECT_DOUBLE_EQ(a.value()[2], 1.044e5);
}

TEST(ParseArrhenius, FewerThanThreeTokensReturnsEmpty) {
  EXPECT_FALSE(parse_arrhenius("1.0 0.0").has_value());
  EXPECT_FALSE(parse_arrhenius("").has_value());
}

TEST(ParseArrhenius, NonNumericTailReturnsEmpty) {
  // The last three tokens are species, not rate coefficients.
  EXPECT_FALSE(parse_arrhenius("H2O + O").has_value());
}

TEST(ParseReactionLine, BasicReversible) {
  auto r = parse_reaction_line("H2+O2<=>H2O+O  1.0e13 0.0 5000.0");
  ASSERT_TRUE(r.has_value());
  EXPECT_TRUE(r->reversible);
  ASSERT_EQ(r->reactants.size(), 2U);
  EXPECT_EQ(r->reactants[0].name, "H2");
  EXPECT_EQ(r->reactants[1].name, "O2");
  ASSERT_EQ(r->products.size(), 2U);
  EXPECT_EQ(r->products[0].name, "H2O");
  EXPECT_EQ(r->products[1].name, "O");
  EXPECT_EQ(r->third_body, third_body_kind::none);
}

TEST(ParseReactionLine, IrreversibleArrow) {
  auto r = parse_reaction_line("CH4+O2=>CO2+H2O  1.0e12 0.0 0.0");
  ASSERT_TRUE(r.has_value());
  EXPECT_FALSE(r->reversible);
}

TEST(ParseReactionLine, ArrheniusParsed) {
  auto r = parse_reaction_line("H2+OH<=>H+H2O  2.20e8 1.51 3430.0");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->arrhenius.has_value());
  EXPECT_DOUBLE_EQ(r->arrhenius.value()[0], 2.20e8);
  EXPECT_DOUBLE_EQ(r->arrhenius.value()[1], 1.51);
  EXPECT_DOUBLE_EQ(r->arrhenius.value()[2], 3430.0);
}

TEST(ParseReactionLine, FortranExponentInRateTail) {
  auto r = parse_reaction_line("H + O2 => OH + O    1.2D+14   0.0   16800.0");
  ASSERT_TRUE(r.has_value());
  ASSERT_TRUE(r->arrhenius.has_value());
  EXPECT_DOUBLE_EQ(r->arrhenius.value()[0], 1.2e14);
}

TEST(ParseReactionLine, FalloffMarkersClassifyReaction) {
  auto r = parse_reaction_line("H+OH(+M)<=>H2O(+M)  2.5e13 0.234 -114.0");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->reactants.size(), 2U);
  EXPECT_EQ(r->reactants[0].name, "H");
  EXPECT_EQ(r->reactants[1].name, "OH");
  ASSERT_EQ(r->products.size(), 1U);
  EXPECT_EQ(r->products[0].name, "H2O");
  EXPECT_EQ(r->third_body, third_body_kind::falloff);
  EXPECT_EQ(r->collider, "M");
}

TEST(ParseReactionLine, SpecificColliderClassifiesFalloff) {
  auto r = parse_reaction_line("H+OH(+N2)<=>H2O(+N2)  2.5e13 0.234 -114.0");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r->third_body, third_body_kind::falloff);
  EXPECT_EQ(r->collider, "N2");
}

TEST(ParseReactionLine, BareMClassifiesMixture) {
  auto r = parse_reaction_line("H2+M<=>H+H+M  4.577e19 -1.4 104400.0");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->reactants.size(), 1U);
  EXPECT_EQ(r->reactants[0].name, "H2");
  // Products: two H entries from "H+H", M classified instead of listed.
  ASSERT_EQ(r->products.size(), 2U);
  EXPECT_EQ(r->products[0].name, "H");
  EXPECT_EQ(r->products[1].name, "H");
  EXPECT_EQ(r->third_body, third_body_kind::mixture);
  EXPECT_EQ(r->collider, "M");
}

TEST(ParseReactionLine, CoefficientExpandedInProducts) {
  auto r = parse_reaction_line("H2O2<=>2OH  2.0e12 0.9 48749.0");
  ASSERT_TRUE(r.has_value());
  ASSERT_EQ(r->products.size(), 1U);
  EXPECT_EQ(r->products[0].name, "OH");
  EXPECT_DOUBLE_EQ(r->products[0].coefficient, 2.0);
  EXPECT_EQ(r->products[0].expanded, (std::vector<std::string>{"OH", "OH"}));
}

TEST(ParseReactionLine, RawLineStored) {
  std::string_view line = "H+OH<=>H2O  1.0 0.0 0.0";
  auto r = parse_reaction_line(line);
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r->raw, line);
}

TEST(ParseReactionLine, InlineCommentStripped) {
  auto r = parse_reaction_line("H+OH<=>H2O  1.0 0.0 0.0 ! provenance note");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(r->raw, "H+OH<=>H2O  1.0 0.0 0.0");
  ASSERT_TRUE(r->arrhenius.has_value());
}

TEST(ParseReactionLine, EmptyLineReturnsEmpty) {
  EXPECT_FALSE(parse_reaction_line("").has_value());
  EXPECT_FALSE(parse_reaction_line("   ").has_value());
  EXPECT_FALSE(parse_reaction_line("! just a comment").has_value());
}

TEST(ParseReactionLine, NoArrowReturnsEmpty) {
  EXPECT_FALSE(parse_reaction_line("ELEMENTS H O C").has_value());
}

TEST(ParseReactionLine, MissingRateTailKeepsAllProducts) {
  // With no numeric tail the whole remainder is product text; the old blind
  // "strip the last three tokens" behavior would have eaten "H2O + O".
  auto r = parse_reaction_line("H2 + O2 <=> H2O + O");
  ASSERT_TRUE(r.has_value());
  EXPECT_FALSE(r->arrhenius.has_value());
  ASSERT_EQ(r->products.size(), 2U);
  EXPECT_EQ(r->products[0].name, "H2O");
  EXPECT_EQ(r->products[1].name, "O");
}

TEST(ParseReactionLine, ShortProductSideWithoutRates) {
  auto r = parse_reaction_line("CH4+O2=>CO2");
  ASSERT_TRUE(r.has_value());
  EXPECT_FALSE(r->arrhenius.has_value());
  ASSERT_EQ(r->products.size(), 1U);
  EXPECT_EQ(r->products[0].name, "CO2");
}

TEST(FormatReaction, NoRateTailYieldsReactionOnly) {
  auto r = parse_reaction_line("CH4 + O2 => CO2");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(format_reaction(*r), "CH4+O2=>CO2");
}

TEST(FormatReaction, MarkersCoefficientsAndArrowNormalized) {
  // '<=>' normalizes to '=', the mixture marker reads as one more species,
  // and coefficients keep their compact spelling.
  auto r = parse_reaction_line("H2+M<=>2H+M");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(format_reaction(*r), "H2+M=2H+M");

  auto f = parse_reaction_line("H + CH3 (+M) = CH4 (+M)");
  ASSERT_TRUE(f.has_value());
  EXPECT_EQ(format_reaction(*f), "H+CH3(+M)=CH4(+M)");
}

TEST(FormatReaction, ArrheniusColumnsAligned) {
  auto r = parse_reaction_line("H+CH3(+M)=CH4(+M)  1.27e16 -0.63 383.0");
  ASSERT_TRUE(r.has_value());
  std::string text = format_reaction(*r);
  EXPECT_NE(text.find("H+CH3(+M)=CH4(+M)"), std::string::npos);
  // Fixed-width columns: 13 characters per value, sign included, so rows
  // stay aligned whether or not a value is negative.
  EXPECT_NE(text.find("  1.27000E+16"), std::string::npos);
  EXPECT_NE(text.find(" -6.30000E-01"), std::string::npos);
  EXPECT_NE(text.find("  3.83000E+02"), std::string::npos);
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
