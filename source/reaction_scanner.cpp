
#include <array>
#include <string>
#include <string_view>

#include "reaction_scanner.hpp"
#include "string_utils.hpp"

namespace ckgrep {

bool starts_with_ci(std::string_view s, std::string_view prefix) {
  if (s.size() < prefix.size()) {
    return false;
  }
  return utils::to_lower(s.substr(0, prefix.size())) == utils::to_lower(prefix);
}

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
    if (std::string_view inner = utils::trim(inside);
        !inner.empty() && inner.front() == '+') {
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
    std::string_view t = utils::trim(tok);
    if (t.empty()) {
      return;
    }
    utils::coefficient_token parsed = utils::parse_coefficient(t);
    std::string_view species = utils::trim(parsed.species);
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

size_t rate_tail_start(std::string_view rest) {
  size_t end = rest.size();
  size_t tokens_found = 0;
  size_t boundary = rest.size();

  while (tokens_found < 3) {
    // Skip trailing whitespace.
    while (end > 0 && utils::is_space(rest[end - 1])) {
      --end;
    }
    if (end == 0) {
      break;
    }
    while (end > 0 && !utils::is_space(rest[end - 1])) {
      --end;
    }
    boundary = end;
    ++tokens_found;
  }
  return tokens_found == 3 ? boundary : rest.size();
}

std::string_view product_portion(std::string_view rest) {
  return rest.substr(0, rate_tail_start(rest));
}

bool is_reaction_line(std::string_view line) {
  std::string_view t = utils::trim(line);
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

bool parse_reaction_line(std::string_view line, const size_t line_number, reaction& out) {
  std::string_view t = utils::trim(line);
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

  out.raw = std::string(utils::trim(line));
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
    std::string_view text_part = utils::trim(line.substr(bang + 1));
    if (text_part.empty()) {
      return;
    }
    result.push_back(
        {std::string(utils::trim(line)), line_number, std::string(text_part)}
    );
  });

  return result;
}
}  // namespace ckgrep
