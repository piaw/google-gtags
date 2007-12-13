#!/usr/bin/python2.4
#
# Copyright 2007 Google Inc. All Rights Reserved.
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

"""A stripped down local version of gentags.

Outputs the name of a file in /tmp directory that contains the tag file that
can be loaded by local gtags server.
"""

__author__ = 'stephenchen@google.com (Stephen Chen)'

import commands
import optparse
import os

option_parser = optparse.OptionParser()

option_parser.add_option(
    '--etags',
    dest='etags',
    default='/usr/pubsw/bin/etags',
    help='Path to etags binary')
option_parser.add_option(
    '--etags_to_tags',
    dest='etags_to_tags',
    default='./etags_to_tags.py',
    help='Path to etags_to_tags binary')
option_parser.add_option(
    '--rtags',
    dest='rtags',
    default='./rtags.py',
    help='Path to rtags binary')
option_parser.add_option(
    '--callgraph',
    dest='callgraph',
    default=False,
    help='Whether to generate callgraph tags.')
option_parser.add_option(
    "--output_file",
    dest="output_file",
    default="",
    help="Tags output file.")
option_parser.add_option(
    '--tmp_file_prefix',
    dest='tmp_file_prefix',
    default='/tmp/gtags.',
    help='Prefix for temp files.')

(options, argv) = option_parser.parse_args()

def GenerateUUID():
  """Call unix uuid command to generate a universal unique id.

  Returns: string containing the output of uuidgen program.
  """
  return commands.getoutput('uuidgen')


def GenerateTmpFileName():
  """Generate a filename in /tmp dir that does not conflict with any existing
  files.

  Returns: string containing a filename in /tmp dir that is safe to write to.
  """
  while True:
    uuid = GenerateUUID()
    filename = options.tmp_file_prefix + uuid
    if not os.path.exists(filename):
      return filename


def Index(files, callgraph, output_file):
  """Call etags/rtags and etags_to_tags on the specified files.

  Outputs results to the specified output_file.

  Args:
    files: list of files to be indexed.
    callgraph: whether to generate callgraph tags.
    output_file: file to output to.

  Returns: name of a file that contains the tags.
  """
  tmp_output = GenerateTmpFileName()
  if callgraph:
    cmd = '%s --output=%s --append %s' % (options.rtags, tmp_output,
                                          ' '.join(files))
    callers_string = '--callers'
  else:
    cmd = '%s -o %s -a %s' % (options.etags, tmp_output,
                              ' '.join(files))
    callers_string = '--nocallers'

  print cmd

  (stdin, stdout, stderr) = os.popen3(cmd)
  (pid, status) = os.wait()
  if status != 0:
    print 'tags generation failed: %s' % stderr.read()
    return ''

  trans_cmd = ('%s --input_file=%s --output_file=%s --guess_language %s' %
               (options.etags_to_tags, tmp_output, output_file, callers_string))

  print trans_cmd

  (stdin, stdout, stderr) = os.popen3(trans_cmd)
  (pid, status) = os.wait()
  if status != 0:
    print 'tags translation failed: %s' % stderr.read()
    return ''
  os.remove(tmp_output)
  return output_file


def main():
  if options.output_file:
    output_file = options.output_file
  else:
    output_file = GenerateTmpFileName()

  print Index(argv, options.callgraph, output_file)

if __name__ == '__main__':
  main()
