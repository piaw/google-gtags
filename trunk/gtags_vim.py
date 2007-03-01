#!/usr/bin/python2.2
#
# Copyright 2004 Google Inc.
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
__version__ = """$Id: //depot/opensource/gtags/gtags_vim.py#5 $"""

import vim
import os
import sys
import re
import socket

sys.path.append(vim.eval('g:google_tags_pydir'))

import gtags
import gtags_google

VIM_TO_GTAGS_TYPES = {
  'c' : 'c++',
  'cpp' : 'c++',
  'java' : 'java',
  'python' : 'python',
  'sawzall' : 'szl'
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


def SoftenServerErrors(f, default):
  """
  Calls f (with no arguments) and returns its result. If f raises a
  socket error, generates a VIM warning messages and returns the
  specified default value instead.
  """
  try:
    return f()
  except socket.error:
    vim.command('echohl WarningMsg')
    vim.command('echomsg "GTAGS server down"')
    vim.command('echohl None')
    return default


def FindExactTag(name):
  # Find a tag using exact match
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_tag(filetype,
                                                   name,
                                                   0,
                                                   vim.eval('g:decipher_protofiles'),
                                                   'vi'),
                            [])


def FindTagByPattern(pattern):
  # Find a tag using regexp
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_matching_tags(filetype,
                                                             pattern,
                                                             0,
                                                             vim.eval('g:decipher_protofiles'),
                                                             'vi',
                                                             0),
                            [])


def FindExactTagCallers(name):
  # Find a tag caller using exact match
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_tag(filetype,
                                                   name,
                                                   1,
                                                   vim.eval('g:decipher_protofiles'),
                                                   'vi'),
                            [])

def FindTagCallersByPattern(pattern):
  # Find a tag caller using regexp
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.find_matching_tags(filetype,
                                                             pattern,
                                                             1,
                                                             vim.eval('g:decipher_protofiles'),
                                                             'vi',
                                                             0),
                            [])

def SearchTagSnippets(pattern):
  # Find a tag searching the snippet
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.search_tag_snippets(filetype,
                                                              pattern,
                                                              0,
                                                              'vi',
                                                              0),
                            [])

def SearchTagCallersSnippets(pattern):
  # Find a tag caller searching the snippet
  filetype = GetFiletype()
  return SoftenServerErrors(lambda: gtags.search_tag_snippets(filetype,
                                                              pattern,
                                                              1,
                                                              'vi',
                                                              0),
                            [])

def ResolveFilename(fnam):
  # The client's google3 directory is assumed to be the top-most
  # ancestor of the current directory named google3. If there is no
  # ancestor of the current directory named google3, then we just use
  # the current directory.
  google3 = re.sub('/google3/.*', '/google3/', os.getcwd() + '/')

  # Filenames supplied by the tags server are relative to google3. For
  # each file, we first look for it under the client's google3
  # directory, and then the various /home/build google3 directorys (in
  # order of decreasing permission requirements).  The first one we can
  # find is the file name that gets written to the output file.  If
  # neither can be found, then the original relative file name is
  # written.
  path = [google3,
          '/home/build/public/google3/',
          '/home/build/nonconf/google3/',
          '/home/build/google3/']

  for dir in path:
    full_fnam = os.path.join(dir, fnam)
    if os.path.exists(full_fnam):
      return full_fnam
  # could not find any existing file..
  # if this is a proto file, then try resolving it as a protodevel too..
  if fnam.endswith(".proto"):
      return ResolveFilename(fnam+"devel")

  return fnam

def SaveTagFile(tags):
  # Write out the tags file
  # The file name for output is read from the vim variable g:google_tags.
  tagfile = vim.eval('g:google_tags')
  f = open(tagfile, 'w')
  for tag in tags:
    fnam = ResolveFilename(tag.filename_)
    print >> f, '%s\t%s\t%s' % (tag.tag_, fnam, tag.lineno_)

# Backwards compatibility
# TODO(leandrog): delete if no one uses this
def WriteTagFile(name):
  WriteTagFileExactMatch(name)

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
    fnam = ResolveFilename(tag.filename_)
    print >>f, '%s:%s:%s' % (fnam, tag.lineno_, tag.snippet_)
