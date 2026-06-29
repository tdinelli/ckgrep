
#include <algorithm>
#include <string>
#include <vector>

#include "matcher.hpp"
#include "string_utils.hpp"

namespace ckgrep {

namespace {

// Expand a side's species_amount list into one name per occurrence: a species
// with an integer coefficient >= 1 (e.g. "2H") contributes that many entries,
// so a query of "H+H" matches a reaction side written as "2H" and vice versa.
// A non-integer coefficient (e.g. "0.5O2") contributes exactly one entry, the
// same as today's species-presence behavior.
std::vector<std::string> expand_species(const std::vector<species_amount>& side) {
  std::vector<std::string> expanded;
  expanded.reserve(side.size());
  for (const species_amount& sa : side) {
    int count = utils::expansion_count(sa.coefficient);
    for (int i = 0; i < count; ++i) {
      expanded.push_back(sa.name);
    }
  }
  return expanded;
}

// Find an injective assignment of query tokens to distinct species: each token
// is matched to a species no other token uses. Classic bipartite matching via
// augmenting paths. Sides have ~2-4 species, so this is trivially fast.
bool can_assign(const query_side& side, const std::vector<std::string>& species) {
  const size_t token_count = side.tokens.size();
  if (token_count > species.size()) {
    return false;
  }

  // assigned_to[s] = index of the token currently using species s, or -1.
  std::vector<int> assigned_to(species.size(), -1);

  // `seen` marks species visited during the search for ONE top-level token. It
  // must be shared across the augmenting-path recursion (Kuhn's algorithm) and
  // reset once per top-level token -- not per recursive call.
  std::vector<char> seen(species.size(), 0);

  auto try_match = [&](int token, auto&& self) -> bool {
    for (size_t s = 0; s < species.size(); ++s) {
      if (seen[s] != 0) {
        continue;
      }
      if (!side.tokens[token]->matches(species[s])) {
        continue;
      }
      seen[s] = 1;
      if (assigned_to[s] == -1 || self(assigned_to[s], self)) {
        assigned_to[s] = token;
        return true;
      }
    }
    return false;
  };

  for (size_t token = 0; token < token_count; ++token) {
    std::fill(seen.begin(), seen.end(), 0);
    if (!try_match(static_cast<int>(token), try_match)) {
      return false;
    }
  }
  return true;
}

bool side_matches(
    const query_side& side,
    const std::vector<species_amount>& species,
    match_mode mode
) {
  std::vector<std::string> expanded = expand_species(species);
  if (!can_assign(side, expanded)) {
    return false;
  }
  if (mode == match_mode::exact && side.tokens.size() != expanded.size()) {
    return false;
  }
  return true;
}

}  // namespace

bool matches(const query& q, const reaction& r, match_mode mode) {
  if (q.any) {
    return side_matches(*q.any, r.reactants, mode) ||
           side_matches(*q.any, r.products, mode);
  }

  bool ok = true;
  if (q.reactants) {
    ok = ok && side_matches(*q.reactants, r.reactants, mode);
  }
  if (q.products) {
    ok = ok && side_matches(*q.products, r.products, mode);
  }
  return ok;
}
}  // namespace ckgrep
