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
#include "reaction_scanner.hpp"

namespace ckgrep {
TEST(IsReactionLine, AcceptsReactionRejectsKeywordAndCommentLines) {
  EXPECT_TRUE(is_reaction_line("H+O2=OH+O  1.0 0.0 0.0"));
  EXPECT_FALSE(is_reaction_line(""));
  EXPECT_FALSE(is_reaction_line("! comment"));
  EXPECT_FALSE(is_reaction_line("REACTIONS"));
  EXPECT_FALSE(is_reaction_line("TROE /0.7 100.0 1000.0/"));
  EXPECT_FALSE(is_reaction_line("low /1.0 2.0 3.0/"));  // keywords match any case
}

TEST(ScanReactions, StampsOneBasedLineNumbers) {
  const std::string_view text =
      "! header comment\n"
      "H2+O2<=>H2O+O  1.0e13 0.0 5000.0\n"
      "LOW /1.0 2.0 3.0/\n"
      "CH4+O2=>CO2+H2O  1.0e12 0.0 0.0\n";
  std::vector<parsed_reaction> reactions = scan_reactions(text);
  ASSERT_EQ(reactions.size(), 2U);
  EXPECT_EQ(reactions[0].line_number, 2U);
  EXPECT_EQ(reactions[1].line_number, 4U);
}

TEST(ScanReactions, InlineCommentStrippedFromRaw) {
  std::vector<parsed_reaction> reactions =
      scan_reactions("H+OH=H2O 1.0 0.0 0.0 ! note\n");
  ASSERT_EQ(reactions.size(), 1U);
  EXPECT_EQ(reactions[0].raw, "H+OH=H2O 1.0 0.0 0.0");
}

TEST(ScanReactions, FalloffClassificationSurvivesScanning) {
  std::vector<parsed_reaction> reactions =
      scan_reactions("H+O2(+M)=HO2(+M)   5.0e12  0.0  0.0\n");
  ASSERT_EQ(reactions.size(), 1U);
  EXPECT_EQ(reactions[0].third_body, third_body_kind::falloff);
  EXPECT_EQ(reactions[0].collider, "M");
}

// A small mechanism exercising every way a line can be commented: a prose
// comment, a commented-out reaction, a live reaction with a prose trailing
// comment, and a plain live reaction.
constexpr std::string_view search_fixture =
    "! CH4 oxidation block\n"
    "!H2+OH=H2O+H 1.0 0.0 0.0\n"
    "H+OH=H2O 1.0 0.0 0.0 ! H2O formation note\n"
    "CH4+O2=CO2+H2O 1.0 0.0 0.0\n";

TEST(SearchText, ReactionHitsArriveInLineOrder) {
  query q = parse_query("H2O");
  std::vector<search_hit> hits = search_text(search_fixture, q, {});
  ASSERT_EQ(hits.size(), 2U);
  EXPECT_EQ(hits[0].line_number, 3U);
  EXPECT_EQ(hits[1].line_number, 4U);
}

TEST(SearchText, LineMatchingAsReactionAndCommentIsOneHit) {
  // The live reaction matches AND its trailing comment is a commented-out
  // reaction that also matches; the line must be reported exactly once.
  query q = parse_query("H2O");
  search_options opts;
  opts.search_comments = true;
  std::vector<search_hit> hits =
      search_text("H+OH=H2O 1.0 0.0 0.0 !H2+O=H2O 1.0 0.0 0.0\n", q, opts);
  ASSERT_EQ(hits.size(), 1U);
}

TEST(SearchText, CommentedOutReactionFoundWithFlagInLineOrder) {
  query q = parse_query("H2O");
  search_options opts;
  opts.search_comments = true;
  std::vector<search_hit> hits = search_text(search_fixture, q, opts);
  ASSERT_EQ(hits.size(), 3U);
  EXPECT_EQ(hits[0].line_number, 2U);  // commented-out, before the live reactions
  EXPECT_EQ(hits[1].line_number, 3U);
  EXPECT_EQ(hits[2].line_number, 4U);
}

TEST(SearchText, ProseCommentsNeverMatch) {
  // Line 1 mentions CH4 as prose and line 3 carries "H2O formation note";
  // neither text parses as a reaction, so -c must not turn them into hits.
  query q = parse_query("CH4");
  search_options opts;
  opts.search_comments = true;
  std::vector<search_hit> hits = search_text(search_fixture, q, opts);
  ASSERT_EQ(hits.size(), 1U);
  EXPECT_EQ(hits[0].line_number, 4U);
}

TEST(SearchText, CommentedOutReactionsIgnoredWithoutFlag) {
  query q = parse_query("H2");  // only the commented-out reaction involves H2
  EXPECT_TRUE(search_text(search_fixture, q, {}).empty());
}

TEST(SearchText, TrailingCommentedOutReactionFound) {
  // A commented-out alternative stashed after a live reaction is findable
  // even though the live reaction itself does not match.
  query q = parse_query("CH3");
  search_options opts;
  opts.search_comments = true;
  std::vector<search_hit> hits =
      search_text("H+O2=OH+O 1.0 0.0 0.0 !CH4+M=CH3+H+M 1.0 0.0 0.0\n", q, opts);
  ASSERT_EQ(hits.size(), 1U);
  EXPECT_EQ(hits[0].line_number, 1U);
}

TEST(SearchText, CommentedOutReactionOwnTrailingCommentTruncated) {
  // The commented-out reaction carries its own trailing comment; parsing
  // must stop at the second '!' for the reaction to be recognized.
  query q = parse_query("CH3");
  search_options opts;
  opts.search_comments = true;
  std::vector<search_hit> hits =
      search_text("!CH4+M=CH3+H+M 1.0 0.0 0.0 ! disabled\n", q, opts);
  ASSERT_EQ(hits.size(), 1U);
}

TEST(SearchText, HitTextIsFullLineWithComment) {
  query q = parse_query("H2O");
  std::vector<search_hit> hits = search_text(search_fixture, q, {});
  ASSERT_EQ(hits.size(), 2U);
  EXPECT_EQ(hits[0].text, "H+OH=H2O 1.0 0.0 0.0 ! H2O formation note");
}

TEST(SearchText, PrettyHitCarriesFormattedReaction) {
  query q = parse_query("H2O");
  search_options opts;
  opts.pretty = true;
  std::vector<search_hit> hits = search_text(search_fixture, q, opts);
  ASSERT_EQ(hits.size(), 2U);
  // Normalized reaction, aligned rate columns, trailing comment gone.
  EXPECT_NE(hits[0].text.find("H+OH=H2O"), std::string::npos);
  EXPECT_NE(hits[0].text.find("1.00000E+00"), std::string::npos);
  EXPECT_EQ(hits[0].text.find('!'), std::string::npos);
}

TEST(SearchText, PrettyCommentedOutReactionLosesItsBang) {
  query q = parse_query("H2");
  search_options opts;
  opts.search_comments = true;
  opts.pretty = true;
  std::vector<search_hit> hits = search_text(search_fixture, q, opts);
  ASSERT_EQ(hits.size(), 1U);
  EXPECT_EQ(hits[0].line_number, 2U);
  EXPECT_NE(hits[0].text.find("H2+OH=H2O+H"), std::string::npos);
  EXPECT_EQ(hits[0].text.find('!'), std::string::npos);
}

TEST(SearchText, ExactModeForwardedToMatcher) {
  query q = parse_query("CH4=");
  search_options contains_opts;
  EXPECT_EQ(search_text(search_fixture, q, contains_opts).size(), 1U);
  search_options exact_opts;
  exact_opts.mode = match_mode::exact;
  // "CH4+O2=..." has a leftover O2 reactant, so exact mode must reject it.
  EXPECT_TRUE(search_text(search_fixture, q, exact_opts).empty());
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
