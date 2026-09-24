# vim-ckgrep

Search CHEMKIN reaction mechanisms by chemistry from Vim or Neovim: `:Ckgrep`
runs [ckgrep](https://github.com/tdinelli/ckgrep) and puts the matching
reaction lines in the quickfix list — `<Enter>` jumps to the hit,
`:cnext`/`:cprev` cycle through matches.

```vim
:Ckgrep "OH+CH4="                   " search the current buffer's file
:Ckgrep! "OH+CH4="                  " search the whole directory, recursively
:Ckgrep -e "H+O2=OH+O" kinetics/    " explicit files/dirs always win
:Ckgrep -c "CH3O2=CH2O+OH" mech.CKI
```

Requires the `ckgrep` binary (prebuilt archives on the
[releases page](https://github.com/tdinelli/ckgrep/releases/latest)). If it
is not on your `$PATH`:

```vim
let g:ckgrep_executable = '/opt/ckgrep/bin/ckgrep'
```

## Install

The plugin lives in the `editor/vim` subdirectory of the ckgrep repository.

### From a CMake install

Building `ckgrep` from source with `-DINSTALL_VIM_PLUGIN=ON` installs this
plugin into `<prefix>/share/ckgrep/vim/` alongside the binary in
`<prefix>/bin/`, so a plugin manager can point straight at the installed
prefix instead of a separate git checkout:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DINSTALL_VIM_PLUGIN=ON
cmake --build build
cmake --install build --prefix "$HOME/.tools/ckgrep"
```

With [lazy.nvim](https://github.com/folke/lazy.nvim):

```lua
{
  dir = vim.fn.expand("~/.tools/ckgrep/share/ckgrep/vim"),
  name = "vim-ckgrep",
  cmd = "Ckgrep",
  init = function()
    vim.g.ckgrep_executable = vim.fn.expand("~/.tools/ckgrep/bin/ckgrep")
  end,
}
```

### From a git checkout

With [vim-plug](https://github.com/junegunn/vim-plug):

```vim
Plug 'tdinelli/ckgrep', { 'rtp': 'editor/vim' }
```

With [lazy.nvim](https://github.com/folke/lazy.nvim):

```lua
{
  "tdinelli/ckgrep",
  config = function()
    vim.opt.rtp:append(vim.fn.stdpath("data") .. "/lazy/ckgrep/editor/vim")
  end,
}
```

Manually (Vim 8+ native packages):

```bash
git clone https://github.com/tdinelli/ckgrep ~/ckgrep-src
mkdir -p ~/.vim/pack/plugins/start
ln -s ~/ckgrep-src/editor/vim ~/.vim/pack/plugins/start/vim-ckgrep
```

Manually (neovim)
```bash
git clone https://github.com/tdinelli/ckgrep ~/ckgrep-src
mkdir -p ~/.local/share/nvim/site/pack/plugins/start
ln -s ~/ckgrep-src/editor/vim ~/.local/share/nvim/site/pack/plugins/start/vim-ckgrep
```

Then `:helptags ALL` once, and `:help ckgrep` for the documentation.
