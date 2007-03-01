#!/usr/bin/python2.4
#
# Copyright 2005 Google Inc. All Rights Reserved.
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


import optparse
import os

option_parser = optparse.OptionParser()

option_parser.add_option("--base_path",
                         dest="base_path",
                         default="",
                         help="Path to use for opening original source files.")
option_parser.add_option("--etags",
                    dest="etags",
                    default="/usr/pubsw/bin/etags",
                    help="Path to etags binary.")

option_parser.add_option("--rtags",
                         dest="rtags",
                         default="./rtags.py",
                         help="Path to rtags.py.")

option_parser.add_option("--etags_to_tags",
                         dest="etags_to_tags",
                         default="./etags_to_tags.py",
                         help="Path to etags_to_tags.py")

etags_to_tags_commands = {
  "cpp.tags"            : [ "TAGS", "c++", False ],
  "cpp.callers.tags"    : [ "TAGSC", "c++", True ],
  "java.tags"           : [ "JTAGS", "java", False ],
  "java.callers.tags"   : [ "JTAGSC", "java", True ],
  "python.tags"         : [ "PTAGS", "python", False ],
  "python.callers.tags" : [ "PTAGSC", "python", True ],
  }

def main():
  (options, args) = option_parser.parse_args()
  # TODO(stephenchen): investigate if this still applies with option_parser
  # these commands have to be defined inside main so that FLAGS.etags will have
  # the right value
  etags_commands = {
    'TAGS' : r"find . -regex '.*\.\(cc\|c\|h\|lex\)' "
             r"| xargs " + options.etags + " -o TAGS -a",
    'JTAGS' : r"find . -regex '.*\.\(java\)' "
              r"| xargs " + options.etags + " -o JTAGS -a",
    'PTAGS' : r"find .  -regex '.*\.\(py\)' "
              r"| xargs " + options.etags + " -o PTAGS -a",
    'TAGSC' : r"find . -regex '.*\.\(cc\|c\|h\|lex\)' | "
           r"xargs " + options.rtags + " --output=TAGSC "
           r"--append",
    'JTAGSC' : r"find . -regex '.*\.\(java\)' | "
           r"xargs " + options.rtags + " --output=JTAGSC "
           r"--append",
    'PTAGSC' : r"find . -regex '.*\.\(py\)' | "
           r"xargs " + options.rtags + " --output=PTAGSC "
           r"--append",
    }

  for target in [ 'TAGS', 'JTAGS', 'PTAGS',
                  'TAGSC' , 'JTAGSC' , 'PTAGSC' ]:
    print 'generating tag file: ' + target
    os.system('rm -f %s' % target)
    result = os.system(etags_commands[target])
    if result != 0:
      print 'errors generating tag file for %s' % target

  for target in etags_to_tags_commands.keys():
    [ infile, language, callers ] = etags_to_tags_commands[target]
    if callers:
      callers_string = "--callers"
    else:
      callers_string = "--nocallers"
    print 'translating tag file: ' + target
    os.system('rm -f %s' % target)
    os.popen((options.etags_to_tags + " --language=%s %s --corpus_name=%s --input_file=%s --output_file=- --base_path=%s | gzip > %s.gz") % (language, callers_string, target, infile, options.base_path, target))

if __name__ == '__main__':
  main()
