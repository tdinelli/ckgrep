[![Build](https://github.com/tdinelli/ckgrep/actions/workflows/build.yml/badge.svg)](https://github.com/tdinelli/ckgrep/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/tdinelli/ckgrep)](https://github.com/tdinelli/ckgrep/releases/latest)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

![ckgrep banner](assets/ckgrep-banner.png)

`ckgrep` (short for "CHEMKIN grep", or "chemical kinetics grep" if you prefer your acronyms spelled out) is a grep-like utility aware of some nuances of the CHEMKIN standard format that allows searching chemical reactions through a standard formalization of the chemical reactions as text. In principle, a sufficiently unhinged regular expression, one that accounts for stoichiometric coefficients, reaction arrows, third bodies, fall-off markers, and inline comments, all while not matching `CH4` against `CH4O`, piped into plain `grep` could do the same thing:

```bash
grep -nE '^[[:space:]]*([0-9.]*[[:space:]]*[A-Za-z0-9()+-]+[[:space:]]*\+[[:space:]]*)*[0-9.]*[[:space:]]*CH4[[:space:]]*(\+|\(\+|=)' mechanism.CKI
```

That finds reactions with `CH4` as a reactant. Want it as a product too, or paired with a specific co-reactant, or to also skip commented-out lines? Keep extending it. In practice, nobody wants to be the person maintaining that regex, so `ckgrep` already understands the chemistry wired into the CHEMKIN sintax or a CHEMKIN-like syntax and lets you write `"CH4="` instead.

## Build & install

Prebuilt binaries for Linux (x86_64, arm64), macOS (universal), and Windows are attached to every [GitHub release](https://github.com/tdinelli/ckgrep/releases/latest); each archive contains the `ckgrep` binary, the LICENSE, and this README. To build from source instead:

Requirements: CMake >= 3.20 and a C++ compiler with `<format>`/`<filesystem>` support (GCC >= 13, Clang >= 17, AppleClang >= 15, or MSVC >= 19.29). Dependencies (`argparse`, plus `googletest` when testing is enabled) are fetched automatically by CMake at configure time.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The `ckgrep` binary is produced at `build/source/ckgrep`.

### Configure options

| Option | Default | Effect |
|--------|---------|--------|
| `BUILD_TESTING` | `ON` | Build the test suite (`ckgrep_tests`), run with `ctest`. |
| `BUILD_DOC` | `OFF` | Enable the `docs` target building the PDF manual. |
| `ENABLE_COVERAGE` | `OFF` | Instrument for coverage and enable the `coverage` target (Clang/AppleClang only). |

### Tests

Built by default; run the suite with:

```bash
ctest --test-dir build --output-on-failure
```

### Documentation

The PDF manual (user guide + API reference) requires Doxygen and a LaTeX toolchain (`pdflatex`, `makeindex`):

```bash
cmake -S . -B build -DBUILD_DOC=ON
cmake --build build --target docs
```

The manual lands at `build/docs/latex/refman.pdf`.

### Coverage

Source-based coverage needs Clang (or AppleClang) and the `llvm-profdata`/`llvm-cov` tools:

```bash
cmake -S . -B build -DENABLE_COVERAGE=ON
cmake --build build
cmake --build build --target coverage
```

This runs the test suite, prints a per-file summary, and writes an HTML report to `build/tests/coverage-html/index.html`.

### Install

```bash
cmake --install build                          # system prefix, e.g. /usr/local
cmake --install build --prefix "$HOME/.local"  # custom prefix
```

Installs the `ckgrep` binary into `<prefix>/bin`, and the LICENSE — plus the PDF manual when it has been built — into `<prefix>/share/doc/ckgrep/`.

## Usage

```bash
ckgrep "<query>" [files...]
```

- No `files` given: recursively searches the current directory.
- One or more files given: searches exactly those files.
- One or more directories given: recursively searches every regular file found inside them.

Flags:

- `-e`, `--exact`: require an exact species match instead of "contains".
- `-c`, `--comments`: also match commented-out reactions (text after `!` that parses as a matching reaction); see [Commented-out reactions](#commented-out-reactions--c).
- `-p`, `--pretty`: reformat matches from the parsed reaction instead of printing the raw line; see [Pretty output](#pretty-output--p).
- `-h`, `--help`: show usage.
- `-v`, `--version`: show the version.

> Always quote the query. Characters like `=`, `*`, and `?` are shell
> metacharacters, so an unquoted query (e.g. `ckgrep =CATECHOL file.CKI`) gets
> interpreted by your shell before `ckgrep` ever sees it.

Exit status: `0` when at least one line matched, `1` when nothing matched or the invocation was invalid, `2` when the query cannot be parsed.

When stdout is a terminal, file names and line numbers are colorized with grep's palette (magenta name, green number, cyan separators). Piped or redirected output stays plain, and setting the [`NO_COLOR`](https://no-color.org) environment variable disables colors everywhere.

## Query syntax

A query is one or more species tokens, optionally split across a reaction arrow (`=`, `=>`, or `<=>`) to anchor the match to reactants and/or products. Tokens on the same side are joined with `+` and are all required (AND); tokens may use `*`/`?` glob wildcards, and matching is case-insensitive by default.

| Query | Meaning |
|-------|---------|
| `CH4` | CH4 participates, on either side |
| `CH4+O2` | CH4 *and* O2, each on either side |
| `OH+CH4=` | OH and CH4 together as reactants |
| `=CO2` | CO2 among the products |
| `O+CH4=OH` | O and CH4 as reactants *and* OH as a product |
| `=2H` | two H atoms among the products, however the file spells it (`2H` or `H+H`) |
| `C3H5*` | any species starting with C3H5, either side |
| `?C3H7` | NC3H7 or IC3H7: `?` matches exactly one character |
| `M` | third-body reactions (e.g. `HCO+M=H+CO+M`) |
| `(+M)` | fall-off reactions, any collider |
| `(+N2)` | fall-off reactions with collider N2 |

All examples below run against the mechanisms bundled in `examples/` and show real output.

### A species, anywhere

Without an arrow the query matches the species on either side of the reaction:

```
$ ckgrep "CH4" examples/TOT_HT_LT_SOOT_NOX.CKI
330: H+CH3(+M)=CH4(+M)          1.2700e+16   -0.630       383.00
335: H+CH4=H2+CH3               6.1400e+05    2.500      9587.00
337: O+CH4=OH+CH3               1.0200e+09    1.500      8600.00
339: OH+CH4=H2O+CH3             5.8300e+04    2.600      2190.00
... (491 lines in total)
```

### Anchoring to a side

Text left of an arrow constrains the reactants, text right of it the products; a blank side is unconstrained:

```
$ ckgrep "OH+CH4=" examples/TOT_HT_LT_SOOT_NOX.CKI
339: OH+CH4=H2O+CH3             5.8300e+04    2.600      2190.00
```

Anchoring to the products is not exclusive: a species matched on the right may also appear as a reactant elsewhere in the same line:

```
$ ckgrep "=CO2" examples/TOT_HT_LT_SOOT_NOX.CKI
285: O+CO(+M)=CO2(+M)           1.3620e+10    0.000      2384.00
289: OH+CO=H+CO2                7.0150e+04    2.053      -355.70
292: OH+CO=H+CO2                5.7570e+12   -0.664       331.80
295: HO2+CO=OH+CO2              1.5700e+05    2.180     17940.00
... (376 lines in total)
```

Both sides can be constrained at once:

```
$ ckgrep "O+CH4=OH" examples/TOT_HT_LT_SOOT_NOX.CKI
337: O+CH4=OH+CH3               1.0200e+09    1.500      8600.00
```

### Stoichiometric coefficients

Coefficients are understood, not matched as text: `H+H` and `2H` are the same thing, in the query and in the mechanism. A single query finds the products however the file spells them (and `"=2H"` returns exactly the same lines):

```
$ ckgrep "=H+H" examples/C3-V4.0.1-FULL.CKI
1818: H2+M<=>H+H+M     4.57700000E+019 -1.40000000E+000 +1.04400000E+005 [...]
2222: CH2+O2=>CO2+2H  4.62E+09    9.93E-01    2.00E+02 [...]
2229: CH2+O=>CO+H+H   +5.00000000E+013 +0.00000000E+000 +0.00000000E+000 [...]
...
```

### Glob wildcards

`*` matches any run of characters, `?` exactly one — handy for isomer families named by convention:

```
$ ckgrep "C3H5*" examples/TOT_HT_LT_SOOT_NOX.CKI
1287: C3H8+C3H5-A=IC3H7+C3H6    7.9400e+11    0.000     16200.00
1309: C3H8+C3H5-A=NC3H7+C3H6    7.9400e+11    0.000     20500.00
1353: CH2(S)+C2H4=H+C3H5-A      8.2000e+19   -2.060      1150.00
...
```

Unlike a literal token, a glob is anchored to the whole species name — `?C3H7` matches the two propyl radical isomers, NC3H7 and IC3H7, but never inside a longer name like NC3H7O2:

```
$ ckgrep "H+?C3H7=C3H8" examples/TOT_HT_LT_SOOT_NOX.CKI
1263: H+NC3H7=C3H8               1.0000e+14    0.000         0.00
1265: H+IC3H7=C3H8               1.0000e+14    0.000         0.00
```

### Third bodies and fall-off

A bare `M` token or a `(+M)`/`(+species)` group in the query is not a species: it constrains the *kind* of reaction. CHEMKIN distinguishes plain reactions (`H+O2=HO2`), third-body reactions (`H2+M=2H+M`), and pressure-dependent fall-off reactions (`H+O2(+M)=HO2(+M)`), and so does the query. A query without a marker matches all three kinds; adding a marker narrows the search:

```
$ ckgrep "=CH4(+M)" examples/TOT_HT_LT_SOOT_NOX.CKI
330: H+CH3(+M)=CH4(+M)          1.2700e+16   -0.630       383.00
1055: CH3CHO(+M)=CO+CH4(+M)     2.7200e+21   -1.740     86355.00
```

A marker can even stand alone: `ckgrep "(+M)" file.CKI` lists every fall-off reaction, `ckgrep "M" file.CKI` every third-body reaction. Following chemical convention, `(+M)` in a query matches any fall-off collider (including specific ones like `(+N2)`), while a specific collider in the query — `(+N2)`, glob-able like any species token — matches only reactions written with that collider.

### Exact mode

By default `"CH4"` also matches species like `CH4O`; pass `-e`/`--exact` to require the whole reaction to match the query exactly, with no leftover species:

```
$ ckgrep -e "O2+CH3=O+CH3O" examples/TOT_HT_LT_SOOT_NOX.CKI
410: O2+CH3=O+CH3O              7.5460e+12    0.000     28320.00
```

Exact mode extends to third bodies: without a marker, `-e` matches only reactions that have *no* third body, so `-e "H+O2=HO2"` does not match the fall-off reaction `H+O2(+M)=HO2(+M)` — write `-e "H+O2(+M)=HO2"` for that.

### Searching many files

Searching multiple files or a directory prefixes each match with its source path — and anything CHEMKIN-like matches, including the reaction equations inside a Cantera YAML export:

```
$ ckgrep "C3H5-A" examples/
examples/C3-V4.0.1-FULL.yaml:35485: - equation: C3H8 + C3H5-A <=> NC3H7 + C3H6  # Reaction 736
examples/C3-V4.0.1-FULL.yaml:35511: - equation: C3H8 + C3H5-A <=> IC3H7 + C3H6  # Reaction 743
examples/TOT_HT_LT_SOOT_NOX.CKI:1287: C3H8+C3H5-A=IC3H7+C3H6    7.9400e+11    0.000     16200.00
... (1384 lines in total)
```

### Commented-out reactions (`-c`)

Mechanism files accumulate history: superseded rates are often left in place, commented out next to their replacement. With `-c`, text behind a `!` is run through the same reaction pipeline as live lines — a commented-out reaction that matches the query is reported, while prose comments never match. The C3 mechanism keeps an older estimate of the CH3O2 decomposition rate commented out right above the current one:

```
$ ckgrep -c "CH3O2=CH2O+OH" examples/C3-V4.0.1-FULL.CKI
2328: !CH3O2=CH2O+OH  8.25E+2 0.85 39000.0 !\Author: SP !\Ref: Villano [...]
2330: CH3O2=CH2O+OH    4.31E+19   -3.86   36270
```

### Pretty output (`-p`)

Instead of the raw line, `-p` re-renders each hit from the parsed reaction: comments dropped entirely, the arrow normalized (`=` reversible, `=>` irreversible), third-body markers re-attached, the reaction padded to a constant width, and the Arrhenius triple in fixed-width scientific columns so consecutive hits columnize:

```
$ ckgrep -p "H+H" examples/TOT_HT_LT_SOOT_NOX.CKI
225: H2+M=2H+M                                                                 4.57700E+19   -1.40000E+00    1.04400E+05
382: O2+CH2=>2H+CO2                                                            2.64000E+12    0.00000E+00    1.50000E+03
384: O+CH2=>2H+CO                                                              5.00000E+13    0.00000E+00    0.00000E+00
...
```

Combined with `-c`, a commented-out reaction is rendered clean, without its `!` — the messy provenance tail from the example above becomes:

```
$ ckgrep -p -c "CH3O2=CH2O+OH" examples/C3-V4.0.1-FULL.CKI
2328: CH3O2=CH2O+OH                                                             8.25000E+02    8.50000E-01    3.90000E+04
2330: CH3O2=CH2O+OH                                                             4.31000E+19   -3.86000E+00    3.62700E+04
```

---

## License

Copyright © 2026 Timoteo Dinelli

This library is distributed under the **GNU General Public License v3.0 or later**. See the [`LICENSE`](LICENSE) file for the full text.

*CRECK Modeling Lab — Politecnico di Milano*  
*Department of Chemistry, Materials, and Chemical Engineering*  
*P.zza Leonardo da Vinci 32, 20133 Milano, Italy*  
*<https://www.creckmodeling.polimi.it>*
