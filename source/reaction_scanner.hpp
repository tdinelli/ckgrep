#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ckgrep {

// One comment found while scanning: the raw line plus the comment text itself
// (everything after '!', trimmed). Covers both full-line comments ("! note")
// and inline trailing comments ("H+H=H2 1e+14 0 0  ! note").
struct comment {
  std::string raw;              // original line, for output
  std::size_t line_number = 0;  // 1-based line in the source file
  std::string text;             // comment text, '!' and surrounding whitespace stripped
};

// One species on a reaction side together with its stoichiometric coefficient,
// e.g. "2H" -> {"H", 2.0}. coefficient is 1.0 when the reaction wrote no
// explicit coefficient for that species.
struct species_amount {
  std::string name;
  double coefficient = 1.0;
};

// One parsed reaction reduced to just what search needs: the original text plus
// the two species multisets. Kinetics (A/b/Ea, PLOG, LOW, TROE, third-body
// efficiencies) are intentionally ignored -- this is a search index, not a
// mechanism loader.
struct reaction {
  std::string raw;                          // original reaction line, for output
  std::size_t line_number = 0;              // 1-based line in the source file
  std::vector<species_amount> reactants;    // species + coeffs; (+M)/M stripped
  std::vector<species_amount> products;     //
  std::string reaction_type;                //
  bool reversible = true;                   // <=> or = vs =>
};

// Decide whether a physical line is a reaction line (has a direction arrow and
// is not a comment / keyword / continuation). Exposed for testing.
bool is_reaction_line(std::string_view line);

// Split one reaction line into a reaction. Returns false if the line is not a
// parseable reaction. Handles: order-independent sides, stoichiometric
// coefficients (2CH3, 2 CH3), (+M)/(+species) fall-off markers, bare M
// third body, and trailing rate coefficients after the equation.
bool parse_reaction_line(std::string_view line, size_t line_number, reaction& out);

// Scan a whole mechanism/text buffer and return every reaction found. Stops
// nothing early; comments and keyword blocks are skipped.
std::vector<reaction> scan_reactions(std::string_view text);

// Scan a whole mechanism/text buffer and return every comment found, both
// full-line ("! note") and inline trailing ("H+H=H2 1e+14 0 0  ! note").
std::vector<comment> scan_comments(std::string_view text);

}  // namespace ckgrep
