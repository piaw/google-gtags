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
import os
import time
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

PYCLIENT_VERSION = 3
PY_CLIENT_IDENTIFIER = "python"

default_corpus = 'google3'
default_language = 'c++'

CONNECT_TIMEOUT = 5.0
DATA_TIMEOUT = 5.0

# GTags mixer settings.
# Command to launch mixer.
MIXER_CMD = "/path/to/gtagsmixer"
# Number of tries before we declare mixer is unreachable.
MIXER_RETRIES = 5
# Waiting time between retries (in ms).
MIXER_RETRY_DELAY = 100
# Default mixer port
MIXER_PORT = 2220

def send_to_server(host, port, command, timeout=DATA_TIMEOUT, proxy=None):
   '''
   Sends command to specified port of gtags server and returns response.

   Use this function only if you want to send a command to a specific data
   center/gtags server. Otherwise, use send_to_server in connection_manager
   which does automatic data center selection and failover based on query
   language and call type.
   '''
   if proxy:
     import socks
     s = socks.socksocket(socket.AF_INET, socket.SOCK_STREAM)
     i = proxy.find(":")
     if i != -1:
       s.setproxy(socks.PROXY_TYPE_HTTP, proxy[0:i], int(proxy[i+1:]))
     else:
       s.setproxy(socks.PROXY_TYPE_HTTP, proxy)
   else:
     s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.setblocking(1)
   s.settimeout(CONNECT_TIMEOUT)
   address = socket.getaddrinfo(host, port, socket.AF_INET,
                                socket.SOCK_STREAM)
   s.connect(address[0][4])
   s.settimeout(timeout)

   # need \r\n to match telnet protocol
   s.sendall(command + '\r\n')

   class SocketReader:
     """SocketReader exposes a single function 'GetResponse' to the caller
     of send_to_server. If the caller is interested in the server response,
     it can call GetResponse(). Otherwise, the caller is free ignore the
     return value"""
     def __init__(self, socket):
       self._socket = socket

     def GetResponse(self):
       buf = StringIO()
       data = s.recv(1024)

       # accumulate all data
       while data:
         buf.write(data)
         data = s.recv(1024)
         signal.alarm(0)
       return buf.getvalue()

   return SocketReader(s)

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
    # A http proxy, e.g: webcache:8080
    self.proxy = None
    self.use_mixer = False
    self.mixer_port = MIXER_PORT
    self.mixer_launched = False

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
      self.current_server[callgraph][language] = \
          self.next_server(language, callgraph)
    return self.current_server[callgraph][language]

  # Switch to the next available server for the language and callgraph
  # Throws NoAvailableServer exception if there is no 'next' avaiable server
  def next_server(self, language, callgraph):
    if (not self.indexes[callgraph].has_key(language)):
      self.indexes[callgraph][language] = 0
    if (not self.lang_to_server[callgraph].has_key(language)):
      self.lang_to_server[callgraph][language] = []
    if (len(self.lang_to_server[callgraph][language]) <=
        self.indexes[callgraph][language]):
      raise NoAvailableServer
    self.current_server[callgraph][language] = \
        self.lang_to_server[callgraph][language] \
            [self.indexes[callgraph][language]]
    self.indexes[callgraph][language] += 1
    return self.current_server[callgraph][language]

  # Send a command to server
  # When self.use_mixer is True, try starting the mixer if called for the
  # first time. After that, send queries to the mixer. If self.user_mixer is not
  # True, figure out hostname and port base on language and callgraph and
  # call send_to_server. If we can't reach current selected sever, move to the
  # next one and try again
  def send_to_server(self, language, is_callgraph, command):
    if not self.proxy and self.use_mixer:
      if not self.mixer_launched:
        os.system(MIXER_CMD + " --port %s" % self.mixer_port + " &")
        time.sleep(0.5)
        self.mixer_launched = True
      for retry_count in xrange(MIXER_RETRIES):
        try:
          return send_to_server("localhost", self.mixer_port,
                                command).GetResponse()
        except socket.error, socket_error:
          if retry_count < MIXER_RETRIES - 1:
            time.sleep(MIXER_RETRY_DELAY / 1000.0)
          else:
            raise socket_error

    callgraph = server_type(is_callgraph)
    while 1:
      host, port = self.selected_server(language, callgraph)
      try:
        return send_to_server(host, port, command, proxy=self.proxy).GetResponse()
      except socket.error:
        self.next_server(language, callgraph)

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

def boolean_lisp_repr(boolean_value):
  """
  Convert a python boolean value (basically anything that evaluates to true or
  false) to s-expression equivalents (nil or t).
  """
  if boolean_value:
    return "t"
  else:
    return "nil"

def make_command(command_type,
                 extra_params = [],
                 language = default_language,
                 callgraph = 0,
                 client_type = "python",
                 client_version = PYCLIENT_VERSION,
                 corpus = default_corpus,
                 current_file = None):
  """
  Given a command-type (string) and list of extra parameters,
  constructs a query string that we can send to the server.

  e.g.
  make_command('foo', [['tag', 40], ['file', 'gtags.py']])
  => '(foo (client-type "python") (client-version 2) (protocol-version 2)
           (tag 40) (file "gtags.py"))'
  """
  client_string = ('(client-type %s) (client-version %s) (protocol-version 2)'
      % (string_lisp_repr(client_type), client_version))

  corp_lang_call_string = '(corpus %s) (language %s) (callers %s)' \
                            % (string_lisp_repr(corpus),
                               string_lisp_repr(language),
                               boolean_lisp_repr(callgraph))

  current_file_string = ''
  if current_file:
    current_file_string = '(current-file %s)' % string_lisp_repr(current_file)

  param_string = ""
  for param in extra_params:
    if isinstance(param[1], str):
      value_repr = string_lisp_repr(param[1])
    else:
      value_repr = repr(param[1])
    param_string += ' (%s %s)' % (param[0], value_repr)
  
  return '(%s %s %s %s %s)' \
         % (command_type, client_string, corp_lang_call_string, 
            current_file_string, param_string)

# returns a list of tags matching id for language language
# Deprecated-- use find_matching_tags_exact instead.
# TODO(leandrog): delete if no one uses this
def find_tag(language,
             id,
             callgraph = 0,
             decipher_genfiles = 0,
             client = 'py',
             corpus = default_corpus,
             current_file=None):
  return find_matching_tags_exact(language,
                                  id,
                                  callgraph,
                                  decipher_genfiles,
                                  client,
                                  corpus,
                                  current_file,
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
    \\, \t and \".

    Returns str, with escaped characters replaced with their original versions.
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
        elif str[i+1] == "t":
          chars.append("\t");
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
                       corpus = default_corpus,
                       current_file = None,
                       ignore_output = 0):
  return do_gtags_command('lookup-tag-prefix-regexp',
                          [["tag", id]],
                          lang,
                          callgraph,
                          decipher_genfiles,
                          client,
                          corpus,
                          current_file,
                          ignore_output)

# Exact match
def find_matching_tags_exact(lang,
                             id,
                             callgraph = 0,
                             decipher_genfiles = 0,
                             client = PY_CLIENT_IDENTIFIER,
                             corpus = default_corpus,
                             current_file = None,
                             ignore_output = 0):
  return do_gtags_command('lookup-tag-exact',
                          [["tag", id]],
                          lang,
                          callgraph,
                          decipher_genfiles,
                          client,
                          corpus,
                          current_file,
                          ignore_output)

# Snippets search
def search_tag_snippets(lang,
                        id,
                        callgraph = 0,
                        client = PY_CLIENT_IDENTIFIER,
                        corpus = default_corpus,
                        current_file = None,
                        ignore_output = 0):
  return do_gtags_command('lookup-tag-snippet-regexp',
                          [["tag", id]],
                          lang,
                          callgraph,
                          0,
                          client,
                          corpus,
                          current_file,
                          ignore_output)

# Check server status with '/' (ping) command
def check_server_status(lang,
                        callgraph = 0,
                        client = PY_CLIENT_IDENTIFIER):
  return connection_manager.send_to_server(lang,
                                           callgraph,
                                           make_command('ping',
                                                        [],
                                                        lang,
                                                        callgraph,
                                                        client))

# Reload tag file using 'file'
def reload_tagfile(file,
                   lang,
                   callgraph = 0,
                   client = PY_CLIENT_IDENTIFIER):
  return connection_manager.send_to_server(lang,
                                           callgraph,
                                           make_command('reload-tags-file',
                                                        [["file", file]],
                                                        lang,
                                                        callgraph,
                                                        client))

# List all tags in a file
def list_tags_for_file(lang, filename, client = PY_CLIENT_IDENTIFIER):
  return do_gtags_command('lookup-tags-in-file',
                          [["file", filename]],
                          lang,
                          0,
                          0,
                          client)

class ErrorMessageFromServer(Exception):
  """Raise an instance of this class if we get a (error ((message "message"))
  from the server"""

def do_gtags_command(command,
                     parameters,
                     lang,
                     callgraph = 0,
                     decipher_genfiles = 0,
                     client = PY_CLIENT_IDENTIFIER,
                     corpus = default_corpus,
                     current_file = None,
                     ignore_output = 0):
  tags_data = connection_manager.send_to_server(
      lang, callgraph, make_command(command,
                                    parameters,
                                    lang,
                                    callgraph,
                                    client,
                                    corpus = corpus,
                                    current_file = current_file))

  if ignore_output:
    return tags_data

  # reserved for genfiles if decipher_genfiles != 0
  prepend_tag_list = []

  tag_list = []
  response_dict = alist_to_dictionary(parse_sexp(tags_data))

  for tag_sexp in response_dict["value"]:
    # Check for error message in the form of
    # [("error",) [[("message"), message],]]
    if "error" == tag_sexp[0][0]:
      error_dict = alist_to_dictionary(tag_sexp[1])
      raise ErrorMessageFromServer(error_dict["message"])

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

def server_reload(host, port, filename):
  # Don't time out on server reloading as it can take a long time
  send_to_server(host,
                 port,
                 make_command('reload-tags-file',
                              [['file', filename]],
                              'nil', 'nil') + '\r\n',
                 timeout = 0)

def server_load_update_file(host, port, filename):
  # Don't time out, because updates can take a few seconds
  send_to_server(host,
                 port,
                 make_command('load-update-file',
                              [['file', filename]],
                              'nil', 'nil') + '\r\n',
                 timeout = 0)
