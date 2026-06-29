#pragma once

#include <string>
#include <vector>

#include "query.hpp"
#include "reaction_scanner.hpp"

namespace ckgrep {

// How strictly a query side must line up with a reaction side.
enum class match_mode {
  contains,  // every query token matches a distinct species; extras allowed
  exact      // as contains, plus no leftover species on the reaction side
};

// Does the reaction satisfy the query under the given mode?
//   - query.reactants / query.products anchor to the matching reaction side
//   - query.any tests reactants OR products (either-side default)
bool matches(const query& q, const reaction& r, match_mode mode);

}  // namespace ckgrep
