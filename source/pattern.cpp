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
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "pattern.hpp"
#include "string_utils.hpp"

namespace ckgrep {
glob_pattern::glob_pattern(std::string source, bool case_sensitive)
    : source_(std::move(source))
    , case_sensitive_(case_sensitive)
{
  normalized_ = case_sensitive_ ? source_ : utils::to_lower(source_);
  is_literal_ =
      source_.find('*') == std::string::npos && source_.find('?') == std::string::npos;
}

bool glob_pattern::matches(std::string_view species) const {
  if (case_sensitive_) {
    return is_literal_ ? species == normalized_
                       : utils::glob_match(normalized_, species);
  }
  std::string lowered = utils::to_lower(species);
  return is_literal_ ? lowered == normalized_ : utils::glob_match(normalized_, lowered);
}

std::unique_ptr<species_pattern>
make_pattern(std::string source, bool case_sensitive, bool use_regex) {
  // use_regex is the reserved seam for a future regex_pattern subclass.
  (void)use_regex;
  return std::make_unique<glob_pattern>(std::move(source), case_sensitive);
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
