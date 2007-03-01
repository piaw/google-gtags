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
Produce index files for reverse call graph data (i.e., function calls)

Generates data in etags format:
etags file format is as follows:
^L
filename,no of chars in file
line snippet 0x7f identifier 0x01 lineno, char offset
"""

__author__ = "piaw@google.com (Piaw Na)"

import string
import sys
import re
import optparse
import os

option_parser = optparse.OptionParser()
option_parser.add_option('--output', dest='output', default='TAGSR',
                         help='output file to put tags data into')
option_parser.add_option('--append',
                         dest='append',
                         default=False,
                         action="store_true",
                         help='append instead of regenerating new tags')
option_parser.add_option('--noappend',
                         dest='append',
                         action="store_false",
                         help='regenerate instead of appending new tags')


funcall = re.compile('([a-zA-Z_][a-zA-Z0-9_]*)\(')

# print all matches on a line
# this is required because function call arguments can themselves
# be function calls, ad-infinitem, and there can be multiple
# function calls on a line for instance f(g) + h(g)
def print_matches(line, lineno, offset):
  dict = {}          # use a dictionary to suppress duplicates
  matches = re.findall(funcall,line)
  for m in matches:
    if not dict.has_key(m):
      # in case the source code contains 0x7f and 0x01, escape them
      line = string.replace(line, '\x7f', '\\0x7f')
      line = string.replace(line, '\x01', '\\0x01')
      print line + chr(0x7f) + m + chr(0x01) + str(lineno) + \
            ',' + str(offset)
      dict[m] = 1

def main():
  (options, argv) = option_parser.parse_args()
  writemode = 'w'

  if options.append:
    writemode = 'a'

  ofile = open(options.output, writemode)
  sys.stdout = ofile

  for filename in argv[1:]:
    lineno = 1
    offset = 0
    try:
      stat = os.open(filename, os.O_RDONLY)
      filesize = os.fstat(stat).st_size
      os.close(stat)
      print '\f'
      print filename + ',' + str(filesize)
      for line in file(filename):
        line = line[0:len(line)-1]
        print_matches(line, lineno, offset)
        lineno += 1
        offset += len(line)
    except:
      print >>sys.stderr, 'error processing file: %s' % filename
  return 0

if __name__ == "__main__":
  main()
