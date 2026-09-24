" Autoloaded implementation of :Ckgrep
" see plugin/ckgrep.vim.

" True when the raw argument string names any file or directory: non-flag
" tokens left after blanking the quoted query that exist on disk. An
" unquoted query (e.g. H+O2) never matches a real path, so it correctly
" falls through to buffer-search instead of being mistaken for a file arg.
function! s:has_file_args(args) abort
  let l:stripped = substitute(a:args, '"[^"]*"\|''[^'']*''', ' ', 'g')
  for l:token in split(l:stripped)
    if l:token[0] !=# '-' && (filereadable(l:token) || isdirectory(l:token))
      return 1
    endif
  endfor
  return 0
endfunction

function! ckgrep#search(bang, args) abort
  let l:exe = get(g:, 'ckgrep_executable', 'ckgrep')
  if !executable(l:exe)
    echohl ErrorMsg
    echomsg 'ckgrep: executable not found: ' . l:exe
          \ . ' (set g:ckgrep_executable to its full path)'
    echohl None
    return
  endif

  " No files named and no bang: search the file behind the current buffer.
  " :Ckgrep! keeps ckgrep's own default, the whole directory recursively.
  let l:args = a:args
  if !a:bang && !s:has_file_args(a:args)
    let l:file = expand('%:.')
    if filereadable(l:file)
      let l:args .= ' ' . shellescape(l:file)
    endif
  endif

  " -H forces the file name on every hit so quickfix entries can jump; the
  " pipe makes ckgrep emit plain (uncolored) output on its own.
  let l:out = systemlist(shellescape(l:exe) . ' -H ' . l:args)

  if v:shell_error == 2
    " Malformed query: ckgrep printed the reason on one line.
    echohl ErrorMsg | echomsg 'ckgrep: ' . join(l:out, ' ') | echohl None
    return
  endif
  if v:shell_error != 0 || empty(l:out)
    echomsg 'ckgrep: no reactions matched'
    return
  endif

  call setqflist([], ' ', {
        \ 'lines': l:out,
        \ 'efm': '%f:%l: %m',
        \ 'title': 'ckgrep ' . l:args,
        \ })
  botright copen
endfunction
