# ckgrep

![ckgrep banner](assets/ckgrep-banner.png)

`ckgrep` is a grep-like utility aware of some nuances of the CHEMKIN standard format that allows searching chemical reactions through a standard formalization of the chemical reactions as text.

## Usage

```bash
ckgrep "<query>" [files...]
```

- No `files` given: recursively searches the current directory.
- One or more files given: searches exactly those files.
- One or more directories given: recursively searches every regular file found inside them.

Flags:

- `-e`, `--exact`: require an exact species match instead of "contains".
- `-c`, `--comments`: also search comment text (full-line `!...` and inline trailing `! ...` text) for the query, as a plain case-insensitive substring match. Independent of species/reaction matching; useful for finding notes, references, or provenance left in the mechanism file.
- `-h`, `--help`: show usage.
- `-v`, `--version`: show the version.

> Always quote the query. Characters like `=`, `*`, and `?` are shell
> metacharacters, so an unquoted query (e.g. `ckgrep =CATECHOL file.CKI`) gets
> interpreted by your shell before `ckgrep` ever sees it.

## Query syntax

A query is one or more species tokens, optionally split across a reaction arrow (`=`, `=>`, or `<=>`) to anchor the match to reactants and/or products. Tokens on the same side are joined with `+` and are all required (AND); tokens may use `*`/`?` glob wildcards, and matching is case-insensitive by default.

- **No arrow** — match a species on *either* side of the reaction:

  ```bash
  ckgrep "CH4" examples/TOT_HT_LT_SOOT_NOX.CKI
  ```
  ```bash
  330: H+CH3(+M)=CH4(+M)                                       1.2700e+16   -0.630       383.00
  335: H+CH4=H2+CH3                                            6.1400e+05    2.500      9587.00
  337: O+CH4=OH+CH3                                            1.0200e+09    1.500      8600.00
  339: OH+CH4=H2O+CH3                                          5.8300e+04    2.600      2190.00
  341: HO2+CH4=H2O2+CH3                                        1.1300e+01    3.740     21010.00
  343: CH4+CH3O2=CH3+CH3O2H                                    9.6000e-01    3.770     17810.00
  Plus many more ...
  ```

- **Arrow, left side filled** — match reactions where the reactants contain the given species:

  ```bash
  ckgrep "OH+CH4=" examples/TOT_HT_LT_SOOT_NOX.CKI
  ```
  ```bash
  339: OH+CH4=H2O+CH3                                          5.8300e+04    2.600      2190.00
  ```

- **Arrow, right side filled** — match reactions where the products contain the given species; this is not exclusive, in the sense that CO2 as a species might also appear as a reactant:

  ```bash
  ckgrep "=CO2" examples/TOT_HT_LT_SOOT_NOX.CKI
  ```
  ```bash
  285: O+CO(+M)=CO2(+M)                                        1.3620e+10    0.000      2384.00
  289: OH+CO=H+CO2                                             7.0150e+04    2.053      -355.70
  292: OH+CO=H+CO2                                             5.7570e+12   -0.664       331.80
  295: HO2+CO=OH+CO2                                           1.5700e+05    2.180     17940.00
  297: O2+CO=O+CO2                                             1.1190e+12    0.000     47700.00
  315: HOCO=H+CO2                                              1.8970e+38   -8.047     34240.00
  357: CO2+CH2(S)=CO2+CH2                                      7.0000e+12    0.000         0.00
  382: O2+CH2=>2H+CO2                                          2.6400e+12    0.000      1500.00
  Plus many more ...
  ```

- **Arrow, both sides filled** — match reactions with the given reactant(s) *and* the given product(s):

  ```bash
  ckgrep "O+CH4=OH" examples/TOT_HT_LT_SOOT_NOX.CKI
  ```
  ```bash
  337: O+CH4=OH+CH3                                            1.0200e+09    1.500      8600.00
  ```

- **Glob wildcards** — `*` matches any run of characters, `?` matches exactly one:

  ```bash
  ckgrep "C3H5*" examples/TOT_HT_LT_SOOT_NOX.CKI
  ```
  ```bash
  1287: C3H8+C3H5-A=IC3H7+C3H6                                  7.9400e+11    0.000     16200.00
  1309: C3H8+C3H5-A=NC3H7+C3H6                                  7.9400e+11    0.000     20500.00
  1353: CH2(S)+C2H4=H+C3H5-A                                    8.2000e+19   -2.060      1150.00
  1361: CH2(S)+C2H4=H+C3H5-A                                    1.0800e+07    1.620     -3174.60
  1385: CH3+C2H3=H+C3H5-A                                       4.1200e+29   -4.950      8000.00
  1393: CH3+C2H3=H+C3H5-A                                       5.7300e+15   -0.770      1195.90
  1417: C3H6=H+C3H5-A                                           9.1600e+74  -17.600    120000.00
  1425: C3H6=H+C3H5-A                                           2.9800e+54  -12.300    101200.00
  Plus many more ...
  ```

- **Exact match (`-e`)** — by default `"CH4"` also matches species like `CH4O`; pass `-e`/`--exact` to require the species name to match in full:

  ```bash
  ckgrep -e "O2+CH3=O+CH3O" examples/TOT_HT_LT_SOOT_NOX.CKI
  ```
  ```bash
  410: O2+CH3=O+CH3O                                           7.5460e+12    0.000     28320.00
  ```

Searching multiple files or a directory prefixes each match with its source path:

```bash
ckgrep "C3H5-A" examples/
```
```
  examples/TOT_HT_LT_SOOT_NOX.CKI:1287: C3H8+C3H5-A=IC3H7+C3H6   7.9400e+11    0.000     16200.00
  examples/TOT_HT_LT_SOOT_NOX.CKI:1309: C3H8+C3H5-A=NC3H7+C3H6   7.9400e+11    0.000     20500.00
  examples/TOT_HT_LT_SOOT_NOX.CKI:1353: CH2(S)+C2H4=H+C3H5-A     8.2000e+19   -2.060      1150.00
  examples/TOT_HT_LT_SOOT_NOX.CKI:1361: CH2(S)+C2H4=H+C3H5-A     1.0800e+07    1.620     -3174.60
  examples/TOT_HT_LT_SOOT_NOX.CKI:1385: CH3+C2H3=H+C3H5-A        4.1200e+29   -4.950      8000.00
  Plus many more ...
```
