
#include <array>
#include <cctype>
#include <string>
#include <string_view>

#include "reaction_scanner.hpp"
#include "string_utils.hpp"

namespace ckgrep {

namespace {

bool is_space(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string_view trim(std::string_view s) {
  size_t b = 0;
  size_t e = s.size();
  while (b < e && is_space(s[b])) {
    ++b;
  }
  while (e > b && is_space(s[e - 1])) {
    --e;
  }
  return s.substr(b, e - b);
}

bool starts_with_ci(std::string_view s, std::string_view prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(s[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
}

// Drop "(+...)" fall-off markers like (+M) or (+N2); keep other parenthesized
// text verbatim (unusual species with parens). The '+' inside "(+M)" must not
// be treated as a species separator, so we strip these groups before splitting.
std::string remove_falloff_markers(std::string_view side) {
  std::string cleaned;
  cleaned.reserve(side.size());
  for (size_t i = 0; i < side.size();) {
    if (side[i] != '(') {
      cleaned.push_back(side[i]);
      ++i;
      continue;
    }
    size_t close = side.find(')', i);
    size_t end = (close == std::string_view::npos) ? side.size() : close + 1;
    std::string_view inside = (close == std::string_view::npos)
                                  ? side.substr(i + 1)
                                  : side.substr(i + 1, close - i - 1);
    if (std::string_view inner = trim(inside); !inner.empty() && inner.front() == '+') {
      i = end;  // fall-off marker: drop
      continue;
    }
    cleaned.append(side.substr(i, end - i));
    i = end;
  }
  return cleaned;
}

void split_side(std::string_view side, std::vector<species_amount>& out) {
  std::string cleaned = remove_falloff_markers(side);

  // Now split on '+'.
  size_t start = 0;
  auto flush = [&](std::string_view tok) {
    std::string_view t = trim(tok);
    if (t.empty()) {
      return;
    }
    utils::coefficient_token parsed = utils::parse_coefficient(t);
    std::string_view species = trim(parsed.species);
    if (species.empty()) {
      return;
    }
    // Bare third-body 'M' is not a species.
    if (species == "M" || species == "m") {
      return;
    }
    out.push_back({std::string(species), parsed.coefficient});
  };

  for (size_t i = 0; i <= cleaned.size(); ++i) {
    if (i == cleaned.size() || cleaned[i] == '+') {
      flush(std::string_view(cleaned).substr(start, i - start));
      start = i + 1;
    }
  }
}

// Arrow location in a reaction string.
struct arrow_info {
  size_t pos = std::string_view::npos;
  size_t len = 0;
  bool reversible = true;
};

// Find the direction arrow, preferring the longest/most specific form.
arrow_info find_arrow(std::string_view t) {
  if (size_t p = t.find("<=>"); p != std::string_view::npos) {
    return {p, 3, true};
  }
  if (size_t p = t.find("=>"); p != std::string_view::npos) {
    return {p, 2, false};
  }
  if (size_t p = t.find('='); p != std::string_view::npos) {
    return {p, 1, true};
  }
  return {};
}

// A CHEMKIN reaction line always ends with exactly three whitespace-delimited
// rate-coefficient tokens (A, n, Ea), regardless of how A is written --
// "174", "2277000000000000.0", and "6.1400e+05" are all valid. Trying to
// classify a token as "numeric" up front is fragile: scientific notation can
// itself contain a '+' (the exponent sign), which is indistinguishable from a
// species-joining '+' by looking at one token alone. Anchoring on a fixed
// count from the end of the line sidesteps that ambiguity entirely: whatever
// the species text looks like, the products always end where the last three
// whitespace-delimited tokens begin.
//
// Returns the start offset of the first of those three trailing tokens, i.e.
// the products text is rest.substr(0, that offset). If fewer than three
// whitespace-delimited tokens exist, returns rest.size() (no rate tail to
// strip).
size_t rate_tail_start(std::string_view rest) {
  size_t end = rest.size();
  size_t tokens_found = 0;
  size_t boundary = rest.size();

  while (tokens_found < 3) {
    // Skip trailing whitespace.
    while (end > 0 && is_space(rest[end - 1])) {
      --end;
    }
    if (end == 0) {
      break;
    }
    while (end > 0 && !is_space(rest[end - 1])) {
      --end;
    }
    boundary = end;
    ++tokens_found;
  }
  return tokens_found == 3 ? boundary : rest.size();
}

// Take the product portion of the text after the arrow: everything before the
// trailing rate-coefficient tokens (see rate_tail_start()).
std::string_view product_portion(std::string_view rest) {
  return rest.substr(0, rate_tail_start(rest));
}

// Walks text one physical line at a time (split on '\n'), invoking
// fn(line, line_number) for each. Shared by scan_reactions() and
// scan_comments() so both see the exact same line/line-number splitting.
template <class Fn>
void for_each_line(std::string_view text, Fn&& fn) {
  size_t line_number = 0;
  size_t pos = 0;

  while (pos <= text.size()) {
    size_t nl = text.find('\n', pos);
    std::string_view line = text.substr(
        pos,
        nl == std::string_view::npos ? std::string_view::npos : nl - pos
    );
    ++line_number;

    fn(line, line_number);

    if (nl == std::string_view::npos) {
      break;
    }
    pos = nl + 1;
  }
}

}  // namespace

bool is_reaction_line(std::string_view line) {
  std::string_view t = trim(line);
  if (t.empty()) {
    return false;
  }
  if (t.front() == '!') {  // comment
    return false;
  }
  // Keyword / sub-data lines that are not reactions.
  static constexpr std::array<std::string_view, 20> keywords = {
      "ELEM",      "ELEMENTS", "SPEC",      "SPECIES", "THER", "THERMO", "REAC",
      "REACTIONS", "END",      "PLOG",      "LOW",     "HIGH", "TROE",   "SRI",
      "REV",       "DUP",      "DUPLICATE", "FORD",    "RORD", "UNITS"
  };
  for (std::string_view kw : keywords) {
    if (starts_with_ci(t, kw)) {
      return false;
    }
  }
  // Must contain a direction arrow / equals to be a reaction.
  return t.find("=>") != std::string_view::npos ||
         t.find("<=>") != std::string_view::npos ||
         t.find('=') != std::string_view::npos;
}

bool parse_reaction_line(std::string_view line, size_t line_number, reaction& out) {
  std::string_view t = trim(line);
  if (!is_reaction_line(t)) {
    return false;
  }

  arrow_info arrow = find_arrow(t);
  if (arrow.pos == std::string_view::npos) {
    return false;
  }

  std::string_view left = t.substr(0, arrow.pos);
  std::string_view rest = t.substr(arrow.pos + arrow.len);

  // The right side is products followed by rate coefficients on the same line;
  // cut at the first numeric (Arrhenius) token.
  std::string_view product_side = product_portion(rest);

  out.raw = std::string(trim(line));
  out.line_number = line_number;
  out.reversible = arrow.reversible;
  out.reactants.clear();
  out.products.clear();
  split_side(left, out.reactants);
  split_side(product_side, out.products);

  return !out.reactants.empty() || !out.products.empty();
}

std::vector<reaction> scan_reactions(std::string_view text) {
  std::vector<reaction> result;

  for_each_line(text, [&](std::string_view line, size_t line_number) {
    // Drop inline comments (everything after '!').
    if (size_t bang = line.find('!'); bang != std::string_view::npos) {
      line = line.substr(0, bang);
    }

    reaction r;
    if (parse_reaction_line(line, line_number, r)) {
      result.push_back(std::move(r));
    }
  });

  return result;
}

std::vector<comment> scan_comments(std::string_view text) {
  std::vector<comment> result;

  for_each_line(text, [&](std::string_view line, size_t line_number) {
    size_t bang = line.find('!');
    if (bang == std::string_view::npos) {
      return;
    }
    std::string_view text_part = trim(line.substr(bang + 1));
    if (text_part.empty()) {
      return;
    }
    result.push_back({std::string(trim(line)), line_number, std::string(text_part)});
  });

  return result;
}

}  // namespace ckgrep
