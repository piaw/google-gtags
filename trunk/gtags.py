#!/usr/bin/python2.4
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
A module for talking to gtags server via python.

This module does the equivalent of gtags.el, but is implemented into python
for possible integration into VIM or other editors.

Because those editors usually don't understand google3 programming conventions,
this module was written as a standard python module.

To use add this to your editor's config:
import sys
import gtags

"""

import socket
import signal
from cStringIO import StringIO

# Server list
# Customize list of server to be something like:
# "c++" : { "definition" : [("gtags.google.com", 2222), (host2, port)],
lang_call_to_server = {
  "c++" : { "definition" : [],
            "callgraph" : [] },
  "java" : { "definition" : [],
             "callgraph" : [] },
  "python" : { "definition" : [],
               "callgraph" : [] } }

# A list of functions that can potentially map a generated file name to its
# source file name
genfile_to_source = []

# String containing characters which are acceptable in a symbol.
SYMBOL_CHARS = "abcdefghijklmnopqrstuvwxyz" + \
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ" + \
               "0123456789" + \
               "+-*/@$%^&_=<>~.?![]{}"

PYCLIENT_VERSION = 2
PY_CLIENT_IDENTIFIER = "python"

default_language = 'c++'

CONNECT_TIMEOUT = 10
DATA_TIMEOUT = 50

def alarm_handler(signum, frame):
  raise RuntimeError, "Timeout!"

# a class to store tags related information
class ETag:
  def __init__(self, filename):
    self.filename_ = filename

  def set_tag(self, tag):
    self.tag_ = tag

  def set_lineno(self, lineno):
    self.lineno_ = lineno

  def set_fileoffset(self, offset):
    self.offset_ = offset

  def set_snippet(self, snippet):
    self.snippet_ = snippet

  def __str__(self):
    return 'tag: ' + self.tag_ + '\n' + 'filename: ' + self.filename_ + \
           '\nlineno: ' + str(self.lineno_) + \
           '\noffset: ' + str(self.offset_) + '\nsnippet: ' + self.snippet_

class Error(Exception):
  pass

class UnparseableSexp(Error):
  "Indicates that a string does not contain a valid s-exp."

class UnexpectedEndOfString(Error):
  "Indicates that a string is terminated by an unescaped backslash."

class NoAvailableServer(Error):
  "Indicates that no gtags server is available to take the request."

def server_type(callgraph):
  "Map a boolean value to dictionary keys"
  if callgraph:
    return "callgraph"
  else:
    return "definition"

class TagsConnectionManager(object):
  '''
  Handles communication with gtags server. Client only needs to supply
  the language and whether the request if for callgraph and
  TagsConnectionManager will determine which server to talk to. It also
  automatically switch to the next available server if the current one
  cannont be reached.
  '''
  def __init__(self):
    self.lang_to_server = {"callgraph" : {},
                           "definition" : {}}
    self.current_server = {"callgraph" : {},
                           "definition" : {}}
    self.indexes = {"callgraph" : {},
                    "definition" : {}}

  # Add a server to the list of known servers
  def add_server(self, language, callgraph, host, port):
    if (not self.lang_to_server[callgraph].has_key(language)):
      self.lang_to_server[callgraph][language] = []
    self.lang_to_server[callgraph][language].append((host, port))

  # Remove a server from the list
  def remove_server(self, language, callgraph, host, port):
    if (not self.lang_to_server[callgraph].has_key(language)):
      return
    self.lang_to_server[callgraph][language].remove((host, port))

  # Return the current selected server for the language and callgraph
  def selected_server(self, language, callgraph):
    if (not self.current_server[callgraph].has_key(language)):
      self.current_server[callgraph][language] = self.next_server(language, callgraph)
    return self.current_server[callgraph][language]

  # Switch to the next available server for the language and callgraph
  # Throws NoAvailableServer exception if there is no 'next' avaiable server
  def next_server(self, language, callgraph):
    if (not self.indexes[callgraph].has_key(language)):
      self.indexes[callgraph][language] = 0
    if (len(self.lang_to_server[callgraph][language]) <=
        self.indexes[callgraph][language]):
      raise NoAvailableServer
    self.current_server[callgraph][language] = self.lang_to_server[callgraph][language][self.indexes[callgraph][language]]
    self.indexes[callgraph][language] += 1
    return self.current_server[callgraph][language]

  # Send a command to server
  # Figure out hostname and port base on language and callgraph
  # and call _send_to_server. If we can't reach current selected
  # sever, move to the next one and try again
  def send_to_server(self, language, is_callgraph, command):
    callgraph = server_type(is_callgraph)
    while 1:
      host, port = self.selected_server(language, callgraph)
      try:
        return self._send_to_server(host, port, command)
      except socket.error:
        self.next_server(language, callgraph)

  def _send_to_server(self, host, port, command):
     '''
     Sends command to specified port of gtags server and returns response.
     '''
     s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
     address = socket.getaddrinfo(host, port, socket.AF_INET,
                               socket.SOCK_STREAM)
     signal.signal(signal.SIGALRM, alarm_handler)
     signal.alarm(CONNECT_TIMEOUT)
     s.connect(address[0][4])
     signal.alarm(0)
     signal.alarm(DATA_TIMEOUT)

     # need \r\n to match telnet protocol
     s.sendall(command + '\r\n')

     buf = StringIO()
     data = s.recv(1024)

     # accumulate all data
     while data:
       buf.write(data)
       data = s.recv(1024)
     signal.alarm(0)
     return buf.getvalue()

# Instance of connection_manager that forwards client requests to gtags server
connection_manager = TagsConnectionManager()

# Register gtags sever with connection_manager as follows
#
for lang in lang_call_to_server.keys():
  for call in lang_call_to_server[lang].keys():
    for host, port in lang_call_to_server[lang][call]:
      connection_manager.add_server(lang, call, host, port)

def string_lisp_repr(string):
  """
  Prints a string as a lisp-readable string literal surrounded by
  double quotes. (Python's repr would do the job, except it likes to
  use single quotes and therefore won't escape double quotes.)
  """
  retval = '"'
  for c in string:
    if c == '\\' or c == '"':
      retval += '\\'
    retval += c
  retval += '"'
  return retval

def make_command(command_type,
                 extra_params = [],
                 client_type = "python",
                 client_version = PYCLIENT_VERSION):
  """
  Given a command-type (string) and list of extra parameters,
  constructs a query string that we can send to the server.

  e.g.
  make_command('foo', [['tag', 40], ['file', 'gtags.py']])
  => '(foo (client-type "python") (client-version 2) (protocol-version 2)
           (tag 40) (file "gtags.py"))'
  """
  param_string = ""
  for param in extra_params:
    if isinstance(param[1], str):
      value_repr = string_lisp_repr(param[1])
    else:
      value_repr = repr(param[1])
    param_string += ' (%s %s)' % (param[0], value_repr)
  return '(%s (client-type %s) (client-version %s) (protocol-version 2)%s)' \
         % (command_type,
            string_lisp_repr(client_type),
            client_version,
            param_string)

# returns a list of tags matching id for language language
# Deprecated-- use find_matching_tags_exact instead.
# TODO(leandrog): delete if no one uses this
def find_tag(language,
             id,
             callgraph = 0,
             decipher_genfiles = 0,
             client = 'py'):
  return find_matching_tags_exact(language,
                                  id,
                                  callgraph,
                                  decipher_genfiles,
                                  client,
                                  0)

def parse_sexp(str):
  """
  Parses a string representation of a s-exp and converts it to a
  python data structure. Can parse strings, integers, symbols and
  lists of those.

  Examples:
  '1450' => "1450"
  '"string"' => "string"
  '()' => []
  '(("a" "b") (105 205))' => [["a", "b"], [105, 205]]

  Transforms improper lists into lists:
  '(1 2 . 3)' => [1, 2, 3]

  Symbols are represented with the symbol name in a one-item tuple:
  'symbol' => ("symbol",)

  Throws UnparseableSexp if there is an error during parsing.
  """

  def unescape_string(str):
    """
    Requires: str does not contain any escaped characters other than
    \\ and \".

    Returns str, with \\ and \" replaced with \ and " respectively.
    """
    chars = []
    i = 0
    while i < len(str):
      if str[i] == "\\":
        if i == len(str) - 1:
          raise UnexpectedEndOfString # Throw an exception if string
                                      # ends in unescaped slash
        if str[i+1] in ["\\", "\""]:
          chars.append(str[i+1])
        i += 1
      else:
        chars.append(str[i])
      i += 1
    return ''.join(chars)

  def parse_sexp_helper(str, start_index):
    """
    Parses a string representation of a s-exp (starting at position
    start_index) into a string, an integer, a symbol-- represented by
    a one-item tuple ("symbolname",)-- or a list. Returns (len,
    answer) containing the number of characters read from the string,
    and the sexp itself.

    Throws UnparseableSexp if there is an error during parsing.
    """
    i = start_index
    while i < len(str) and str[i] == " ": # Skip leading spaces.
      i += 1

    if i == len(str):
      raise UnparseableSexp, "No s-exp was found in input."
    data_start_index = i # This is where the token/list actually starts.

    if str[i] == "(":
      # Parse list
      elements = []
      i += 1
      if i == len(str):
        raise UnparseableSexp, "Input unexpectedly ended inside a list."
      while str[i] != ")":
        while str[i] == " ":
          i += 1
          if i == len(str):
            raise UnparseableSexp, "Input unexpectedly ended inside a list."
        if str[i] == ")":
          pass
        else:
          if str[i] == ".": # Just ignore "." (in improper lists).
            # TODO(psung): Check for spurious "."'s (e.g. more than
            # one, or one anywhere other than the second-to-last
            # position) and throw an exception
            i += 1
          else:
            (skip_ahead, result) = parse_sexp_helper(str, i)
            i += skip_ahead
            elements.append(result)
          if i >= len(str): # No room for closing parenthesis
            raise UnparseableSexp, "Input unexpectedly ended inside a list."
      i += 1
      return (i - start_index, elements)
    elif str[i] == '"':
      # Parse string literal
      i += 1
      if i == len(str):
        raise UnparseableSexp, "Input contains unterminated quoted string."
      while str[i] != '"':
        if str[i] == "\\":
          i += 1 # Skip over the slash.
          if i == len(str):
            raise UnparseableSexp, "Quoted string ends in unescaped backslash."
        i += 1
        if i == len(str):
          raise UnparseableSexp, "Input contains unterminated quoted string."
      try:
        s = unescape_string(str[data_start_index+1:i])
      except UnexpectedEndOfString:
        raise UnparseableSexp, "Quoted string ends in unescaped backslash."
      i += 1
      return (i - start_index, s)
    elif str[i] in "0123456789":
      # Parse integer literal
      # TODO(psung): Handle negative ints, floats, etc. This code is
      # also bad in that it assumes a token is a number if the first
      # char is a digit.
      while i < len(str) and str[i] in "0123456789":
        i += 1
      return (i - start_index, int(str[data_start_index:i]))
    elif str[i] in SYMBOL_CHARS or str[i] == "\\":
      # Parse symbol
      while i < len(str):
        if str[i] not in SYMBOL_CHARS + "\\":
          break
        if str[i] == "\\":
          i += 1 # Skip over the slash.
          if i == len(str):
            raise UnparseableSexp, "Symbol ends in unescaped backslash."
        i += 1
      try:
        s = unescape_string(str[data_start_index:i])
      except UnexpectedEndOfString:
        raise UnparseableSexp, "Symbol ends in unescaped backslash."
      return (i - start_index, (s,))

    raise UnparseableSexp, ("Unexpected character '%s' was found in input; " +
                            "beginning of token/list was expected") % str[i]

  return parse_sexp_helper(str, 0)[1]

def alist_to_dictionary(alist):
  """
  Takes a parsed alist, i.e. of the form
  [[("key1",), value1], ...]
  --which is what you get when you parse ((key1 value1) ...) --
  and returns a dictionary indexed by the key symbols:
  result["key1"] => value1
  """
  # We want [key, value], but the input is [(key,), value]
  return dict([[item[0][0], item[1]] for item in alist])

# Parameters of functions
#
# find_matching_tags, find_matching_tags_exact, search_tag_snippets,
# check_server_status, reload_tagfile, list_tags_for_file and do_gtags_command,
#
# lang = language
# id = tag you are searching for
# callgraph = O:definitions mode, 1: callgraph mode
# decipher_genfiles = 0:No, 1:Yes
# client = Name of the client using this library. Defaults to
#          PY_CLIENT_IDENTIFIER (= "python")
# ignore_output = 0: Parse the output, 1: Do not parse the output

# Regular expression match
def find_matching_tags(lang,
                       id,
                       callgraph = 0,
                       decipher_genfiles = 0,
                       client = PY_CLIENT_IDENTIFIER,
                       ignore_output = 0):
  return do_gtags_command('lookup-tag-prefix-regexp',
                          [["tag", id]],
                          lang,
                          callgraph,
                          decipher_genfiles,
                          client,
                          ignore_output)

# Exact match
def find_matching_tags_exact(lang,
                             id,
                             callgraph = 0,
                             decipher_genfiles = 0,
                             client = PY_CLIENT_IDENTIFIER,
                             ignore_output = 0):
  return do_gtags_command('lookup-tag-exact',
                          [["tag", id]],
                          lang,
                          callgraph,
                          decipher_genfiles,
                          client,
                          ignore_output)

# Snippets search
def search_tag_snippets(lang,
                        id,
                        callgraph = 0,
                        client = PY_CLIENT_IDENTIFIER,
                        ignore_output = 0):
  return do_gtags_command('lookup-tag-snippet-regexp',
                          [["tag", id]],
                          lang,
                          callgraph,
                          0,
                          client,
                          ignore_output)

# Check server status with '/' (ping) command
def check_server_status(lang,
                        callgraph = 0,
                        client = PY_CLIENT_IDENTIFIER):
  return connection_manager.send_to_server(lang,
                                           callgraph,
                                           make_command('ping', [], client))

# Reload tag file using 'file'
def reload_tagfile(file,
                   lang,
                   callgraph = 0,
                   client = PY_CLIENT_IDENTIFIER):
  return connection_manager.send_to_server(lang,
                                           callgraph,
                                           make_command('reload-tags-file',
                                                        [["file", file]],
                                                        client))

# List all tags in a file
def list_tags_for_file(lang, filename, client = PY_CLIENT_IDENTIFIER):
  return do_gtags_command('lookup-tags-in-file',
                          [["file", filename]],
                          lang,
                          0,
                          0,
                          client)

def do_gtags_command(command,
                     parameters,
                     lang,
                     callgraph = 0,
                     decipher_genfiles = 0,
                     client = PY_CLIENT_IDENTIFIER,
                     ignore_output = 0):
  tags_data = connection_manager.send_to_server(lang,
                                                callgraph,
                                                make_command(command,
                                                             parameters,
                                                             client))

  if ignore_output:
    return tags_data

  # reserved for genfiles if decipher_genfiles != 0
  prepend_tag_list = []

  tag_list = []
  response_dict = alist_to_dictionary(parse_sexp(tags_data))

  for tag_sexp in response_dict["value"]:
    tag_dict = alist_to_dictionary(tag_sexp)
    filename = tag_dict["filename"]

    tag = ETag(filename)
    tag.set_snippet(tag_dict["snippet"])
    tag.set_tag(tag_dict["tag"])
    tag.set_lineno(tag_dict["lineno"])
    tag.set_fileoffset(tag_dict["offset"])
    tag_list.append(tag)

    for f in genfile_to_source:
      gentag = f(filename, tag_dict)
      if (gentag):
        if decipher_genfiles != '0': # prepend
          prepend_tag_list.append(gentag)
        else: # append
          tag_list.append(gentag)

  return prepend_tag_list + tag_list

def server_reload(language, callgraph, filename):
  connection_manager.send_to_server(language, callgraph,
    make_command('reload-tags-file', [['file', filename]]) + '\r\n')
