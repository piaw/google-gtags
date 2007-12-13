#!/usr/bin/python2.4
#
# Copyright 2006 Google Inc. All Rights Reserved.
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
Converts Exuberant etags output to a TAGS file.

Takes an etags output file and translates it to the s-expression
format. We also look at the original source files and insert
descriptors corresponding to the #include lines.
"""

__author__ = 'psung@google.com (Phil Sung)'

import optparse
import os
import re
import sys
import time

option_parser = optparse.OptionParser()

option_parser.add_option(
  "--input_file",
  dest="input_file",
  default="-",
  help="Etags output file to process, or '-' for stdin.")
option_parser.add_option(
  "--output_file",
  dest="output_file",
  default="-",
  help="TAGS output file to create, or '-' for stdout.")
option_parser.add_option(
  "--base_path",
  dest="base_path",
  default="",
  help="Base path for looking up original files to search for #includes and "
  "imports. The default is '' which disables this lookup.")
option_parser.add_option(
  "--corpus_name",
  dest="corpus_name",
  default="google3",
  help="Name which describes the tags corpus.")
option_parser.add_option(
  "--language",
  dest="language",
  default="c++",
  help="Language to apply to this TAGS file. This option is supressed when"
  "--guess_language mode is enabled")
option_parser.add_option(
  "--guess_language",
  dest="guess_language",
  default=False,
  action="store_true",
  help="etags_to_tags will attempt to guess the language based on file "
  "extension when this flag is enabled. This has precedence over --language. "
  "We fall back to --language when we do not recognize the file extension.")
option_parser.add_option(
  "--deletion_list",
  dest="deletion_list",
  default=None,
  help="List of files to output as deleted.")
# When True, we make a (call ...) descriptor for each tag rather than
# a (generic-tag ...) descriptor.
option_parser.add_option(
  "--callers",
  dest="callers",
  default=False,
  action="store_true",
  help="Flag the TAGS file as including callers.")
option_parser.add_option(
  "--nocallers",
  dest="callers",
  action="store_false",
  help="Flag the TAGS file as not including callers.")
option_parser.add_option(
  "--noheader",
  dest="header",
  default=True,
  action="store_false",
  help="Do not output the TAGS header.")

(options, args) = option_parser.parse_args()

extension_map = {
  ".cc"              : "c++",
  ".cpp"             : "c++",
  ".c"               : "c++",
  ".h"               : "c++",
  ".lex"             : "c++",
  ".java"            : "java",
  ".py"              : "python",
  }

def string_lisp_repr(string):
  """
  Prints a string as a lisp-readable string literal surrounded by
  double quotes. (Python's repr would do the job, except it likes to
  use single quotes and therefore won't escape double quotes.)
  """
  retval = '"'
  for c in string:
    if c != '\0':
      if c == '\\' or c == '"':
        retval += '\\'
      retval += c
    else:
      # Just ignore strings which contain null's.
      # It may be good to output a warning here.
      return '""'
  retval += '"'
  return retval

def guess_language(filename, default):
  """
  Guess programming language using file extension.
  """
  if options.guess_language:
    return extension_map.get(os.path.splitext(filename)[1], default)
  return default

def write_tagged_lines(filename, tagged_lines, outfile, language, base_path):
  """
  Given a particular FILENAME (relative path) and a list of
  TAGGED_LINES (of the format [[line, offset, descriptor, snippet]
  ...] ), writes the corresponding tag expression to OUTFILE.

  LANGUAGE and BASE_PATH correspond to the flags of the same name.
  """
  language = guess_language(filename, language)
  outfile.write('(file \n  (path %s)\n  (language %s)\n  (contents (' %
                (string_lisp_repr(filename), string_lisp_repr(language)))

  if base_path != "":
    file_fullpath = os.path.join(base_path, filename)

  fileoffset = 0
  # Open the specified file and look for references

  is_first_line = True
  for [lineno, charno, descriptor, snippet] in tagged_lines:
    if not is_first_line:
      outfile.write('\n             ')
    is_first_line = False
    outfile.write('(item (line %s) (offset %s) (descriptor %s) (snippet %s))' %
                  (lineno, charno, descriptor, string_lisp_repr(snippet)))
  outfile.write(')))\n')


def write_tags_file(infile, outfile, language, base_path, callers, corpus_name,
                    deletion_list_filename):
  """
  Reads INFILE (etags format tags file), converts it to new-style TAGS
  and writes that to OUTFILE.

  LANGUAGE, BASE_PATH, CALLERS, CORPUS_NAME and DELETION_LIST
  correspond to the flags of the same name.
  """

  # Write file header
  if options.header:
    outfile.write('(tags-format-version 2)\n')
    outfile.write('(tags-comment "")\n')
    outfile.write('(timestamp %s)\n' % int(time.time()))
    outfile.write('(tags-corpus-name %s)\n' % string_lisp_repr(corpus_name))

  filename = None

  # Read tags from file
  for line in infile:
    # If we read an empty line (not even including \n), then we have been
    # given an empty tags file or we are at EOF, so return.
    if line == "":
      break
    line = line[:-1]
    if line == "\014":  # "^L"
      # Next line contains a file header containing the filename
      file_line = True
    else:
      if file_line:
        # Reading file header. Flush entries from last file first, if possible.
        if filename:
          write_tagged_lines(filename, tagged_lines, outfile, language,
                             base_path)
        [filename, structure_size] = line.split(",")
        tagged_lines = []
      else:
        # Line is in format snippet\177tagname\001line,offset
        snippet_end = line.find("\177")
        tag_end = line.find("\001")
        # no tags (TODO: Find out why)
        if tag_end == -1:
          continue
        snippet = line[0:snippet_end]
        tag = line[snippet_end+1:tag_end]
        [lineno, charno] = line[tag_end+1:].split(",")
        if callers:
          entry_template = '(call (to (ref (name %s))))'
        else:
          entry_template = '(generic-tag (tag %s))'
        tagged_lines.append([lineno,
                             charno,
                             entry_template % string_lisp_repr(tag),
                             snippet])
      file_line = False

  # Flush output again
  if filename:
    write_tagged_lines(filename, tagged_lines, outfile, language, base_path)

  # Write deleted files
  if deletion_list_filename:
    deletion_file = open(deletion_list_filename, "r")
    for line in deletion_file:
      outfile.write("(deleted %s)\n" % string_lisp_repr(line[:-1]))


def main():
  if options.input_file == "-":
    infile = sys.stdin
  else:
    infile = open(options.input_file, "r")
  if options.output_file == "-":
    outfile = sys.stdout
  else:
    outfile = open(options.output_file, "w")

  write_tags_file(infile,
                  outfile,
                  options.language,
                  options.base_path,
                  options.callers,
                  options.corpus_name,
                  options.deletion_list)

  infile.close()
  outfile.close()

if __name__ == '__main__':
  main()
