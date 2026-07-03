# ckgrep User Guide {#mainpage}

`ckgrep` (short for "CHEMKIN grep") is a grep-like utility for CHEMKIN
reaction mechanisms. Instead of matching raw text, it parses every candidate
line into species, direction arrow, third body, and rate coefficients, and
matches the *chemistry*: the query `CH4` finds reactions in which methane
participates, without matching species such as `CH4O`, and without you
maintaining the regular expression that would otherwise be required.

All examples in this guide run against the mechanisms bundled in the
repository's `examples/` directory and show real output.

## Invoking ckgrep

```
ckgrep [options] "<query>" [files...]
```

- No `files` given: recursively searches the current directory.
- One or more files given: searches exactly those files.
- One or more directories given: recursively searches every regular file
  found inside them.

Always quote the query. Characters like `=`, `*`, `(`, and `?` are shell
metacharacters, so an unquoted query (e.g. `ckgrep =CATECHOL file.CKI`) gets
interpreted by your shell before `ckgrep` ever sees it.

Exit status: `0` when at least one line matched, `1` when nothing matched or
the invocation was invalid, `2` when the query cannot be parsed.

When stdout is a terminal, file names and line numbers are colorized with
grep's palette (magenta name, green number, cyan separators). Piped or
redirected output stays plain, and setting the NO_COLOR environment variable
(https://no-color.org) disables colors everywhere.

### Options

| Flag | Effect |
|------|--------|
| `-e`, `--exact` | Require the whole reaction to match the query exactly, instead of "contains". |
| `-c`, `--comments` | Also match commented-out reactions (text after `!` that parses as a matching reaction). |
| `-p`, `--pretty` | Reformat matches from the parsed reaction instead of printing the raw line. |
| `-h`, `--help` | Show usage and exit. |
| `-v`, `--version` | Show the version and exit. |

## Query grammar

A query is one or more species tokens, optionally split across a reaction
arrow (`=`, `=>`, or `<=>`) to anchor the match to reactants and/or
products. Tokens on the same side are joined with `+` and are all required
(AND); tokens may use `*`/`?` glob wildcards, and matching is
case-insensitive.

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

## Worked examples

### A species, anywhere

Without an arrow the query matches the species on either side of the
reaction:

```
$ ckgrep "CH4" examples/TOT_HT_LT_SOOT_NOX.CKI
330: H+CH3(+M)=CH4(+M)          1.2700e+16   -0.630       383.00
335: H+CH4=H2+CH3               6.1400e+05    2.500      9587.00
337: O+CH4=OH+CH3               1.0200e+09    1.500      8600.00
339: OH+CH4=H2O+CH3             5.8300e+04    2.600      2190.00
... (491 lines in total)
```

### Anchoring to a side

Text left of an arrow constrains the reactants, text right of it the
products; a blank side is unconstrained:

```
$ ckgrep "OH+CH4=" examples/TOT_HT_LT_SOOT_NOX.CKI
339: OH+CH4=H2O+CH3             5.8300e+04    2.600      2190.00
```

Both sides can be constrained at once:

```
$ ckgrep "O+CH4=OH" examples/TOT_HT_LT_SOOT_NOX.CKI
337: O+CH4=OH+CH3               1.0200e+09    1.500      8600.00
```

### Stoichiometric coefficients

Coefficients are understood, not matched as text: `H+H` and `2H` are the
same thing, in the query and in the mechanism. A single query finds the
products however the file spells them (and `"=2H"` returns exactly the same
lines):

```
$ ckgrep "=H+H" examples/C3-V4.0.1-FULL.CKI
1818: H2+M<=>H+H+M     4.57700000E+019 -1.40000000E+000 +1.04400000E+005 [...]
2222: CH2+O2=>CO2+2H  4.62E+09    9.93E-01    2.00E+02 [...]
2229: CH2+O=>CO+H+H   +5.00000000E+013 +0.00000000E+000 +0.00000000E+000 [...]
...
```

### Glob wildcards

`*` matches any run of characters, `?` exactly one -- handy for isomer
families named by convention:

```
$ ckgrep "C3H5*" examples/TOT_HT_LT_SOOT_NOX.CKI
1287: C3H8+C3H5-A=IC3H7+C3H6    7.9400e+11    0.000     16200.00
1309: C3H8+C3H5-A=NC3H7+C3H6    7.9400e+11    0.000     20500.00
1353: CH2(S)+C2H4=H+C3H5-A      8.2000e+19   -2.060      1150.00
...
```

Unlike a literal token, a glob is anchored to the whole species name --
`?C3H7` matches the two propyl radical isomers, NC3H7 and IC3H7, but never
inside a longer name like NC3H7O2:

```
$ ckgrep "H+?C3H7=C3H8" examples/TOT_HT_LT_SOOT_NOX.CKI
1263: H+NC3H7=C3H8               1.0000e+14    0.000         0.00
1265: H+IC3H7=C3H8               1.0000e+14    0.000         0.00
```

### Third bodies and fall-off

A bare `M` token or a `(+M)`/`(+species)` group in the query is not a
species: it constrains the *kind* of reaction. A query without a marker
matches plain, third-body, and fall-off reactions alike; adding a marker
narrows the search:

```
$ ckgrep "=CH4(+M)" examples/TOT_HT_LT_SOOT_NOX.CKI
330: H+CH3(+M)=CH4(+M)          1.2700e+16   -0.630       383.00
1055: CH3CHO(+M)=CO+CH4(+M)     2.7200e+21   -1.740     86355.00
```

A marker can stand alone: `ckgrep "(+M)"` lists every fall-off reaction
(63 in this mechanism), `ckgrep "M"` every third-body reaction. Following
chemical convention, `(+M)` matches any fall-off collider, while a specific
collider like `(+N2)` -- glob-able like any species token -- matches only
reactions written with that collider.

### Exact mode

By default a species token also matches inside longer names under the same
side; pass `-e`/`--exact` to require the whole reaction to match the query
exactly, with no leftover species:

```
$ ckgrep -e "O2+CH3=O+CH3O" examples/TOT_HT_LT_SOOT_NOX.CKI
410: O2+CH3=O+CH3O              7.5460e+12    0.000     28320.00
```

Exact mode extends to third bodies: without a marker, `-e` matches only
reactions that have *no* third body, so `-e "H+O2=HO2"` does not match the
fall-off reaction `H+O2(+M)=HO2(+M)` -- write `-e "H+O2(+M)=HO2"` for that.

### Searching many files

Searching multiple files or a directory prefixes each match with its source
path:

```
$ ckgrep "C2H5O2H=" examples/
examples/C3-V4.0.1-FULL.yaml:32062: C2H5O2H<=>C2H5O+OH ...
examples/TOT_HT_LT_SOOT_NOX.CKI:751: C2H5O2H=OH+C2H5O ...
...
```

## Commented-out reactions (-c)

Mechanism files accumulate history: superseded rates are often left in
place, commented out next to their replacement. By default everything
behind a `!` is ignored. With `-c`, commented text is instead run through
the same reaction pipeline as live lines, so a commented-out reaction that
matches the query is reported -- while prose comments never match: a note
that merely *mentions* `CH4` is not a hit, because the note does not parse
as a reaction involving CH4.

The C3 mechanism keeps an older estimate of the CH3O2 decomposition rate
commented out right above the current one. Without `-c` only the live
reaction is found:

```
$ ckgrep "CH3O2=CH2O+OH" examples/C3-V4.0.1-FULL.CKI
2330: CH3O2=CH2O+OH    4.31E+19   -3.86   36270
```

With `-c` the superseded rate surfaces too, provenance note and all:

```
$ ckgrep -c "CH3O2=CH2O+OH" examples/C3-V4.0.1-FULL.CKI
2328: !CH3O2=CH2O+OH  8.25E+2 0.85 39000.0 !\Author: SP !\Ref: Villano [...]
2330: CH3O2=CH2O+OH    4.31E+19   -3.86   36270
```

This also finds commented-out alternatives stashed *after* a live reaction
(`H2+M=H+H+M ... !CH4+M=CH3+H+M ...`), since a trailing comment is tested
as a reaction the same way.

## Pretty output (-p)

Instead of the raw line, `-p` re-renders each hit from the parsed reaction:
comments dropped entirely, the arrow normalized (`=` reversible, `=>`
irreversible), third-body markers re-attached, the reaction padded to a
constant width, and the Arrhenius triple in fixed-width scientific columns
so consecutive hits columnize (the padding is compressed here to fit the
page; real output pads every reaction to 70 characters):

```
$ ckgrep -p "H+H" examples/TOT_HT_LT_SOOT_NOX.CKI
225: H2+M=2H+M            4.57700E+19   -1.40000E+00    1.04400E+05
382: O2+CH2=>2H+CO2       2.64000E+12    0.00000E+00    1.50000E+03
384: O+CH2=>2H+CO         5.00000E+13    0.00000E+00    0.00000E+00
...
```

Combined with `-c`, a commented-out reaction is rendered clean, without its
`!` -- the superseded rate from the previous section becomes:

```
$ ckgrep -p -c "CH3O2=CH2O+OH" examples/C3-V4.0.1-FULL.CKI
2328: CH3O2=CH2O+OH        8.25000E+02    8.50000E-01    3.90000E+04
2330: CH3O2=CH2O+OH        4.31000E+19   -3.86000E+00    3.62700E+04
```

## API reference

The remaining chapters are generated from the source code and document the
C++ internals: the query engine (query.hpp, matcher.hpp), the CHEMKIN
reaction parser (reaction_parser.hpp), and the line scanner / search engine
(reaction_scanner.hpp).
