" Copyright 2004 Google Inc.
" All Rights Reserved.
"
" This program is free software; you can redistribute it and/or
" modify it under the terms of the GNU General Public License
" as published by the Free Software Foundation; either version 2
" of the License, or (at your option) any later version.
"
" This program is distributed in the hope that it will be useful,
" but WITHOUT ANY WARRANTY; without even the implied warranty of
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
" GNU General Public License for more details.
"
" You should have received a copy of the GNU General Public License
" along with this program; if not, write to the Free Software
" Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"
"
" Vim script for handling google tags. Note that it depends on
" gtags_vim.py, and requires a vim executable with python support
" compiled in.
"
" $Id: //depot/opensource/gtags/gtags.vim#5 $
"
" Author: Laurence Gonsalves (laurence@google.com)
"         Leandro Groisman (leandrog@google.com)
"
" Tip: you might want to map CTRL-] to use Gtag. You can do that like
" this:
"   nmap <C-]> :call Gtag(expand('<cword>'))<CR>

" Set Vim compatibility for this script
let s:cpo_save = &cpo
set cpo&vim

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Configuration variables - you can override these in your .vimrc
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
if ! exists('g:google_tags_corpus')
  let g:google_tags_corpus = 'google3'
endif
if ! exists('g:google_tags_default_filetype')
  let g:google_tags_default_filetype = 'c++'
endif
" Set the proxy(e.g.: proxy:port) if you need to access gtags server via proxy.
if ! exists('g:google_tags_proxy')
  let g:google_tags_proxy = ''
endif
" GTags mixer settings. Set g:google_tags_user_mixer to 1 to enable mixer.
if ! exists('g:google_tags_use_mixer')
  let g:google_tags_use_mixer = 1
endif
if ! exists('g:google_tags_mixer_port')
  let g:google_tags_mixer_port = 2220
endif
" Gtlist format: 'long' for 2-line or 'short' for 1-line
if ! exists('g:google_tags_list_format')
  let g:google_tags_list_format = 'short'
endif
" Gtlist height in lines, or empty string to split window equally
if ! exists('g:google_tags_list_height')
  let g:google_tags_list_height = ''
endif
if ! exists('g:google_tags_list_orientation')
  let g:google_tags_list_orientation = 'horizontal'
endif


""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Setup
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

" create temp tags file - we'll rewrite this whenever GtagWrite is
" called. Note that we add it to the *end* of the tags search path, so a
" local tags file takes precedence
let g:google_tags = tempname()
call system('touch ' . g:google_tags)
exe 'set tags+=' . g:google_tags

let g:google_grep = tempname()
call system('touch ' . g:google_grep)

let g:google_tags_pydir = expand('<sfile>:p:h')
let g:google_tags_vimfile = expand('<sfile>')
let g:google_tags_initialized = 0
" Lazy loading of python code to avoid slow vim startup times
function! GtagInitialize()
  if ! g:google_tags_initialized
    exe 'pyfile ' . substitute(g:google_tags_vimfile, '\.vim$', '_vim', '') . '.py'
    augroup gtags
      autocmd BufReadCmd gtags://* py ReadGtagsFile()
    augroup end
 endif
  let g:google_tags_initialized = 1
endfunction

""""""""""""""""""""""""""""""""
" define python helper functions
""""""""""""""""""""""""""""""""
function! GtagWriteExactMatch(name)
  call GtagInitialize()
  python WriteTagFileExact(vim.eval('a:name'))
endfunction

function! GtagWriteRegExpMatch(name)
  call GtagInitialize()
  python WriteTagFileRegExp(vim.eval('a:name'))
endfunction

function! GtagWriteSnippetMatch(name)
  call GtagInitialize()
  python WriteTagFileSnippets(vim.eval('a:name'))
endfunction

function! GtagWriteExactMatch_callers(name)
  call GtagInitialize()
  python WriteTagFileExact_callers(vim.eval('a:name'))
endfunction

function! GtagWriteRegExpMatch_callers(name)
  call GtagInitialize()
  python WriteTagFileRegExp_callers(vim.eval('a:name'))
endfunction

function! GtagWriteSnippetMatch_callers(name)
  call GtagInitialize()
  python WriteTagFileSnippets_callers(vim.eval('a:name'))
endfunction

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Functions for displaying a list of results (from Gtlist, etc).
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
let g:google_tags_list_buffer = '__GTags__'

" Set up options, input mappings and highlighting for the GTags list window.
function! s:SetListWindowOptions()
  setlocal buftype=nofile
  setlocal bufhidden=delete
  setlocal noswapfile
  setlocal nobuflisted
  setlocal nonumber
  setlocal noro
  if v:version >= 700
    setlocal cursorline
  endif
  nnoremap <buffer> <silent> <CR> :py OpenListTag(close=1,use_stack=1)<CR>
  nnoremap <buffer> <silent> <S-CR> :py OpenListTag(split=1, close=1)<CR>
  nnoremap <buffer> <silent> v :py OpenListTag()<CR><C-W>p
  nnoremap <buffer> <silent> s :py OpenListTag(split=1)<CR><C-W>p
  nnoremap <buffer> <silent> e :py OpenListTag()<CR>
  nnoremap <buffer> <silent> <S-e> :py OpenListTag(split=1)<CR>
  nnoremap <buffer> <silent> <2-LeftMouse> :py OpenListTag()<CR>
  nnoremap <buffer> <silent> <MiddleMouse> <LeftMouse>:py OpenListTag(split=1)<CR>
  nnoremap <buffer> <silent> q :close<CR>
  if has('syntax')
    syntax match gtagsFile /^[^|]*/ nextgroup=gtagsSeparator
    syntax match gtagsSeparator /|/ contained nextgroup=gtagsLine
    syntax match gtagsLine /[0-9]*/ contained
    highlight link gtagsFile Directory
    highlight link gtagsLine LineNr
  endif
endfunction

" Switch to the GTags list window, creating it and its buffer if necessary.
function! s:SwitchToListWindow()
  let buf = g:google_tags_list_buffer
  let height = g:google_tags_list_height
  let vert = (g:google_tags_list_orientation == 'vertical') ? 'vertical' : ''
  " If the window exists, just jump to it
  if bufwinnr(buf) != -1
    exe bufwinnr(buf) . 'wincmd w'
    return
  endif
  " Create a window, using the previous buffer number if a buffer
  " exists or creating a new buffer otherwise.
  if bufnr(buf) != -1
    exe 'silent! bo ' . height . ' ' . vert . ' split +buffer' . bufnr(buf)
  else
    exe 'silent! bo ' . height . ' ' . vert . ' split ' . buf
  endif
  call s:SetListWindowOptions()
endfunction

" Display a list with the results of the given find-tags function.
function! GtagList(findFunction, name)
  call GtagInitialize()
  exe 'python GtagList(' . a:findFunction . '(vim.eval("a:name")))'
endfunction

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" Public functions/command
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" position the cursor under the word and add it to the current
" search list

function! s:MoveCursor(name)
endfunction

"""""""""""""""""""""
" Completion function
"""""""""""""""""""""

function! Gtag_complete(ArgLead, CmdLine, CursorPos)
  call GtagInitialize()
  python GtagComplete()
  return r
endfunction

""""""""""""""""""""
" Commands like tag
""""""""""""""""""""
" Exact matches
function! Gtag(name)
  call GtagWriteExactMatch(a:name)
  exe 'ta ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtag call Gtag("<args>")

" regexp matches
function! Gtagregexp(name)
  call GtagWriteRegExpMatch(a:name)
  exe 'ta ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtagregexp call Gtagregexp("<args>")

" snippet matches
function! Gtagsnippet(name)
  call GtagWriteSnippetMatch(a:name)
  exe 'ta ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtagsnippet call Gtagsnippet("<args>")

" Exact matches - callers
function! Gtagcallers(name)
  call GtagWriteExactMatch_callers(a:name)
  exe 'ta ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtagcallers call Gtagcallers("<args>")

" regexp matches - callers
function! Gtagregexpcallers(name)
  call GtagWriteRegExpMatch_callers(a:name)
  exe 'ta ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtagregexpcallers call Gtagregexpcallers("<args>")

" snippet matches - callers
function! Gtagsnippetcallers(name)
  call GtagWriteSnippetMatch_callers(a:name)
  exe 'ta ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtagsnippetcallers call Gtagsnippetcallers("<args>")

""""""""""""""""""""""
" Command like tselect
""""""""""""""""""""""
" Exact matches
" TODO: doesn't support no-arg version like tselect
function! Gtselect(name)
  call GtagWriteExactMatch(a:name)
  exe 'ts ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtselect call Gtselect("<args>")

" regexp matches
function! Gtselectregexp(name)
  call GtagWriteRegExpMatch(a:name)
  exe 'ts ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtselectregexp call Gtselectregexp("<args>")

" snippet matches
function! Gtselectsnippet(name)
  call GtagWriteSnippetMatch(a:name)
  exe 'ts ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtselectsnippet call Gtselectsnippet("<args>")

" Exact matches - callers
function! Gtselectcallers(name)
  call GtagWriteExactMatch_callers(a:name)
  exe 'ts ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtselectcallers call Gtselectcallers("<args>")

" regexp matches - callers
function! Gtselectregexpcallers(name)
  call GtagWriteRegExpMatch_callers(a:name)
  exe 'ts ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtselectregexpcallers call Gtselectregexpcallers("<args>")

" snippet matches - callers
function! Gtselectsnippetcallers(name)
  call GtagWriteSnippetMatch_callers(a:name)
  exe 'ts ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtselectsnippetcallers call Gtselectsnippetcallers("<args>")

""""""""""""""""""""
" Command like tjump
""""""""""""""""""""
" Exact matches
" TODO: doesn't support no-arg version like tjump
function! Gtjump(name)
  call GtagWriteExactMatch(a:name)
  exe 'tj ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtjump call Gtjump("<args>")

" regexp matches
function! Gtjumpregexp(name)
  call GtagWriteRegExpMatch(a:name)
  exe 'tj ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtjumpregexp call Gtjumpregexp("<args>")

" snippet matches
function! Gtjumpsnippet(name)
  call GtagWriteSnippetMatch(a:name)
  exe 'tj ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtjumpsnippet call Gtjumpsnippet("<args>")

" Exact matches - callers
function! Gtjumpcallers(name)
  call GtagWriteExactMatch_callers(a:name)
  exe 'tj ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtjumpcallers call Gtjumpcallers("<args>")

" regexp matches - callers
function! Gtjumpregexpcallers(name)
  call GtagWriteRegExpMatch_callers(a:name)
  exe 'tj ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtjumpregexpcallers call Gtjumpregexpcallers("<args>")

" snippet matches - callers
function! Gtjumpsnippetcallers(name)
  call GtagWriteSnippetMatch_callers(a:name)
  exe 'tj ' . a:name
  call s:MoveCursor(a:name)
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtjumpsnippetcallers call Gtjumpsnippetcallers("<args>")

""""""""""""""""""""
" sort of like :grep, except that the search is done in gtags, rather
" than in files you specify.
function! Gtgrep(pattern)
  call GtagInitialize()
  python Gtgrep()
  " save options
  let errorformat=&errorformat

  " load 
  let &errorformat='%f|%l|%m'
  copen
  exe 'cgetfile ' . g:google_grep

  " restore options
  let &errorformat=errorformat
endfunction
command! -nargs=1 -complete=custom,Gtag_complete Gtgrep call Gtgrep("<args>")

""""""""""""""""""""
" Commands for displaying a list of results in the GTags list window
command! -nargs=1 -complete=custom,Gtag_complete Gtlist
      \ call GtagList("FindExactTag", "<args>")
command! -nargs=1 -complete=custom,Gtag_complete Gtlistregexp
      \ call GtagList("FindTagByPattern", "<args>")
command! -nargs=1 -complete=custom,Gtag_complete Gtlistsnippet
      \ call GtagList("SearchTagSnippets", "<args>")
command! -nargs=1 -complete=custom,Gtag_complete Gtlistcallers
      \ call GtagList("FindExactTagCallers", "<args>")
command! -nargs=1 -complete=custom,Gtag_complete Gtlistregexpcallers
      \ call GtagList("FindTagCallersByPattern", "<args>")
command! -nargs=1 -complete=custom,Gtag_complete Gtlistsnippetcallers
      \ call GtagList("SearchTagCallersSnippets", "<args>")

""""""""""""""""""""
" Restart GTags mixer
function! Gtrestartmixer()
  call GtagInitialize()
  py RestartMixer()
endfunction
command! -nargs=0 Gtrestartmixer call Gtrestartmixer()

""""""""""""""""""""
" Set GTags corpus
command! -nargs=1 Gtcorpus let g:google_tags_corpus="<args>"


""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
" These are for backwards compatibility with the messed up
" capitalization I used before
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
function! GTag(name)
  call Gtag(a:name)
endfunction
function! GTJump(name)
  call Gtjump(a:name)
endfunction
function! GTSelect(name)
  call Gtselect(a:name)
endfunction

command! -nargs=1 -complete=custom,Gtag_complete GTag call Gtag("<args>")
command! -nargs=1 -complete=custom,Gtag_complete GTJump call Gtjump("<args>")
command! -nargs=1 -complete=custom,Gtag_complete GTSelect call Gtselect("<args>")

" Restore user's compatibility options
let &cpo = s:cpo_save
unlet s:cpo_save
