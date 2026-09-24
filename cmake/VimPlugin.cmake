# -----------------------------------------------------------------------------
# Installs the Vim/Neovim plugin (editor/vim) into
# <prefix>/share/ckgrep/vim/{plugin,autoload,doc}, so a plugin manager can
# point straight at an installed prefix instead of a git checkout, e.g. with
# lazy.nvim:
#
#   {
#     dir = "<prefix>/share/ckgrep/vim",
#     name = "vim-ckgrep",
#     cmd = "Ckgrep",
#     init = function()
#       vim.g.ckgrep_executable = "<prefix>/bin/ckgrep"
#     end,
#   }
# -----------------------------------------------------------------------------
include(GNUInstallDirs)
install(
  DIRECTORY
    ${PROJECT_SOURCE_DIR}/editor/vim/plugin
    ${PROJECT_SOURCE_DIR}/editor/vim/autoload
    ${PROJECT_SOURCE_DIR}/editor/vim/doc
  DESTINATION ${CMAKE_INSTALL_DATADIR}/ckgrep/vim
)
