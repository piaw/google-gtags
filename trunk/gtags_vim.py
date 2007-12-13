# Copyright 2004-2007 Google Inc.
# All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


"""
Python helper script for gtags.vim.

Note that this cannot be executed by a normal Python interpreter. It
imports this vim module, and hence needs to be executed by a Python
interpreter embedded within vim (or gvim).
"""

__author__ = "laurence@google.com (Laurence Gonsalves)"
__version__ = """$Id: //depot/opensource/gtags/gtags_vim.py#8 $"""

import vim
import os
import sys
import re
import socket

sys.path.append(vim.eval('g:google_tags_pydir'))

import gtags

VIM_TO_GTAGS_TYPES = {
  'c' : 'c++',
  'cpp' : 'c++',
  'java' : 'java',
  'python' : 'python'
}

def GetFiletype():
  # determine filetype based on vim's current filetype, then
  # g:google_tags_default_filetype, and then finally assume C++ if we
  # still can't find anything reasonable.
  filetype = vim.eval('&ft')
  filetype = VIM_TO_GTAGS_TYPES.get(filetype)
  if not filetype:
    filetype = vim.eval('g:google_tags_default_filetype')
  if not filetype:
    filetype = 'c++'
  return filetype

def UpdateConnectionSettings():
  UpdateCorpusSetting()
  UpdateProxySetting()
  UpdateMixerSetting()

def UpdateCorpusSetting():
  # Define what set of servers we will use based on the chosen corpus
  # (e.g. 'google3' or 'googleclient_wireless').
  pass
  # gtags_google.define_servers(vim.eval('g:google_tags_corpus'))

def UpdateProxySetting():
  # Update the proxy to be used based on user's setting. First we try
  # the g:google_tags_proxy variable, then we fall back to GTAGS_PROXY
  # environment variable. If no proxy is set, the proxy will be set to
  # None.
  proxy = vim.eval('g:google_tags_proxy')
  if not proxy:
    proxy = os.getenv('GTAGS_PROXY')
  gtags.connection_manager.proxy = proxy

def UpdateMixerSetting():
  # Check whether the user wants to use GTags through the mixer.
  # vim.eval returns string
  use_mixer_string = vim.eval('g:google_tags_use_mixer')
  # use mixer only when google_tags_use_mixer is defined and not 0.
  use_mixer = use_mixer_string and use_mixer_string != '0'
  gtags.connection_manager.use_mixer = use_mixer
  if (use_mixer):
    mixer_port = vim.eval('g:google_tags_mixer_port')
    # Reset only if port changed.
    if (mixer_port and mixer_port != gtags.connection_manager.mixer_port):
      gtags.connection_manager.mixer_port = mixer_port
      gtags.connection_manager.mixer_launched = False

def SoftenServerErrors(f, default):
  """
  Calls f (with no arguments) and returns its result. If f raises a
  socket error, generates a VIM warning messages and returns the
  specified default value instead.
  """
  try:
    return f()
  except (socket.error, gtags.Error, RuntimeError), msg:
    vim.command('echohl Error')
    vim.command('echomsg "Failed to communicate with GTags: ' + str(msg) + '"')
    vim.command('echomsg "You may need to restart the GTags mixer. Please '
                'type :Gtrestartmixer."')
    vim.command('echohl None')
    return default
  except gtags.ErrorMessageFromServer, msg:
    vim.command('echohl Error')
    vim.command('echomsg "' + str(msg) + '"')
    vim.command('echomsg "You may need to restart the GTags mixer. Please '
                'type :Gtrestartmixer."')
    vim.command('echohl None')
    return default
  except:
    return default

def FindExactTag(name):
  # Find a tag using exact match
  UpdateConnectionSettings()
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_tag(filetype,
                                                   name,
                                                   0,
                                                   0,
                                                   'vi',
                                                   vim.eval('g:google_tags_corpus'),
                                                   vim.current.buffer.name),
                            [])


def FindTagByPattern(pattern):
  # Find a tag using regexp
  UpdateConnectionSettings()
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_matching_tags(filetype,
                                                             pattern,
                                                             0,
                                                             0,
                                                             'vi',
                                                             vim.eval('g:google_tags_corpus'),
                                                             vim.current.buffer.name),
                            [])


def FindExactTagCallers(name):
  # Find a tag caller using exact match
  UpdateConnectionSettings()
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_tag(filetype,
                                                   name,
                                                   1,
                                                   0,
                                                   'vi',
                                                   vim.eval('g:google_tags_corpus'),
                                                   vim.current.buffer.name),
                            [])

def FindTagCallersByPattern(pattern):
  # Find a tag caller using regexp

  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_matching_tags(filetype,
                                                             pattern,
                                                             1,
                                                             0,
                                                             'vi',
                                                             vim.eval('g:google_tags_corpus'),
                                                             vim.current.buffer.name),
                            [])

def SearchTagSnippets(pattern):
  # Find a tag searching the snippet
  UpdateConnectionSettings()
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.search_tag_snippets(filetype,
                                                              pattern,
                                                              0,
                                                              'vi',
                                                              vim.eval('g:google_tags_corpus'),
                                                              vim.current.buffer.name),
                            [])

def SearchTagCallersSnippets(pattern):
  # Find a tag caller searching the snippet
  UpdateConnectionSettings()
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.search_tag_snippets(filetype,
                                                              pattern,
                                                              1,
                                                              'vi',
                                                              vim.eval('g:google_tags_corpus'),
                                                              vim.current.buffer.name),
                            [])


_default_resolve_path = None
_resolve_cache = {}

# The base project path for all tag results.
# Useful if your server tags are built with relative paths.
google_tags_proj_path = ""

def ResolveFilename(fnam):
  global google_tags_proj_path
  path = [os.getcwd() + '/']
  if google_tags_proj_path != "":
    path.insert(0, google_tags_proj_path)

  for dir in path:
    full_fnam = os.path.join(dir, fnam)
    if os.path.exists(full_fnam):
      return full_fnam

  return fnam

def ReadGtagsFile():
  """Autocommand handler used to reopen a buffer with a gtags://<path>
  filename with the correct path in the user's Perforce client if the file 
  exists  there, or in /home/build otherwise."""
  original_name = vim.current.buffer.name
  resolved_name = ResolveFilename(original_name[len("gtags://"):])
  vim.command("silent doau BufReadPre " + resolved_name)
  vim.command("silent keepjumps edit " + resolved_name)
  vim.command("silent doau BufReadPost " + resolved_name)
  vim.command("silent bdelete! " + original_name)
  # Refresh buffers menu
  vim.command("if has('gui_running')\nemenu Buffers.Refresh\ menu\nendif")

def SaveTagFile(tags):
  # Write out the tags file
  # The file name for output is read from the vim variable g:google_tags.
  tagfile = vim.eval('g:google_tags')
  f = open(tagfile, 'w')
  for tag in tags:
    print >> f, '%s\tgtags://%s\t%s' % (tag.tag_, tag.filename_, tag.lineno_)

# Methods:
#    WriteTagFileExact, WriteTagFileRegExp, WriteTagFileExact_Callers,
#    WriteTagFileRegExp_Callers, WriteTagFileSnippets,
#    WriteTagFileSnippets_Callers
#
# Write a vi-format tags file based on the supplied symbol name
# The file name for output is read from the vim variable g:google_tags.

def WriteTagFileExact(name): SaveTagFile(FindExactTag(name))

def WriteTagFileExact_callers(name): SaveTagFile(FindExactTagCallers(name))

def WriteTagFileRegExp(pattern): SaveTagFile(FindTagByPattern(pattern))

def WriteTagFileRegExp_callers(pattern): SaveTagFile(FindTagCallersByPattern(pattern))

def WriteTagFileSnippets(pattern): SaveTagFile(SearchTagSnippets(pattern))

def WriteTagFileSnippets_callers(pattern): SaveTagFile(SearchTagCallersSnippets(pattern))


def SortedAndUniquified(seq):
  m = {}
  for v in seq:
    m[v] = 1
  seq = m.keys()
  seq.sort()
  return seq


def GtagComplete():
  arglead = vim.eval('a:ArgLead')

  tags = FindTagByPattern(arglead)
  result = [tag.tag_ for tag in tags]
  # TODO: make sure result has no magic chars?

  result = SortedAndUniquified(result)

  vim.command('let r="%s"' % '\\n'.join(result))


def Gtgrep():
  pattern = vim.eval('a:pattern')
  outfnam = vim.eval('g:google_grep')
  tags = FindTagByPattern(pattern)
  f = open(outfnam, 'w')
  for tag in tags:
    print >> f, 'gtags://%s|%s|%s' % (tag.filename_, tag.lineno_, tag.snippet_)


# Last set of tags found by Gtlist (used in OpenListTag to access tag names).
_list_tags = []


def GtagList(tags):
  global _list_tags
  _list_tags = tags
  if len(tags) == 0:
    vim.command('echohl ErrorMsg')
    vim.command('echomsg "No tags found"')
    vim.command('echohl None')
  elif len(tags) == 1:
    # Go to the single match, also pushing it on the tag stack.
    GoToSingleTag(_list_tags[0])
  else:
    vim.command('call s:SwitchToListWindow()')
    vim.command('setlocal modifiable')
    WriteListBuf(tags)
    vim.command('setlocal nomodifiable')
    vim.command('0')


def WriteListBuf(tags):
  """Write a list of tags to the current buffer, for use in the Gtlist
  series of commands.
  """
  buf = vim.current.buffer
  width = vim.current.window.width
  lines = []
  if vim.eval('g:google_tags_list_format') == 'short':
    for tag in tags:
      lines.append("%s|%s| %s" %
          (tag.filename_, tag.lineno_, tag.snippet_.strip()))
  else:
    for tag in tags:
      reference = '%s|%s|' % (tag.filename_, tag.lineno_)
      num_spaces = max(1, width - len(tag.tag_) - len(reference))
      lines.append('%s%s%s%s' %
          (reference, ' ' * num_spaces, tag.tag_, tag.snippet_))
  buf[:] = lines


def OpenListTag(split=False, close=False, use_stack=False):
  """A function called in the GTags list window to handle a user action.
  Goes to the tag on the current line in the list, with different effects based
  on the arguments.

  Args:
    split: Open the file in a new split window instead of using an existing one.
    close: Also close the GTags results list.
    use_stack: Use the tag stack, as if the user had invoked :tag, instead of
               opening the file manually here.
  """
  tag = _list_tags[int(vim.eval('line(".")')) - 1]
  if use_stack:
    # Switch to the previous window used and run :tag in there.
    vim.command('wincmd p')
    GoToSingleTag(tag)
  else:
    # Switch to an appropriate window and open the file in it.
    file = ResolveFilename(tag.filename_)
    winnr = vim.eval('bufwinnr("' + file + '")')
    if winnr != '-1':
      vim.command(winnr + 'wincmd w')
    else:
      cur_window = vim.eval('winnr()')
      vim.command('wincmd p')
      last_window = vim.eval('winnr()')
      vim.command('wincmd p')
      if last_window == cur_window or split:
        vim.command('split +%s %s' % (tag.lineno_, file))
      else:
        vim.command(last_window + 'wincmd w')
        vim.command('edit +%s %s' % (tag.lineno_, file))
      vim.command('normal z.')
  if close:
    vim.command('bdelete ' + vim.eval('bufnr(g:google_tags_list_buffer)'))


def GoToSingleTag(tag):
  """Go to a particular tag, writing it in the google_tags file by itself."""
  f = open(vim.eval('g:google_tags'), 'w')
  print >> f, '%s\tgtags://%s\t%s' % (tag.tag_, tag.filename_, tag.lineno_)
  f.close() # Needed because we call tag next
  old_tags = vim.eval('&tags')
  vim.command('set tags=' + vim.eval('g:google_tags'))
  vim.command('tag ' + tag.tag_)
  vim.command('set tags=' + old_tags)


def RestartMixer():
  os.system(gtags.MIXER_CMD + ' --replace &')
