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


import re
import optparse
import os

option_parser = optparse.OptionParser()

option_parser.add_option("--base_path",
                         dest="base_path",
                         default="",
                         help="Path to use for opening original source files.")

option_parser.add_option("--etags",
                         dest="etags",
                         default="/usr/bin/etags",
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

target_to_language = {
  "TAGS"                 : "cpp",
  "JTAGS"                : "java",
  "PTAGS"                : "python",
  "TAGSC"                : "cpp",
  "JTAGSC"               : "java",
  "PTAGSC"               : "python",
  }

extensions = {
  "cpp"                 : re.compile("^.*\.(cc|c|h|lex)$"),
  "java"                : re.compile("^.*\.(java)$"),
  "python"              : re.compile("^.*\.(py)$"),
  }

file_list = {
  "cpp"                 : [],
  "java"                : [],
  "python"              : [],
  }

def find_source_files(dirs):
  for directory in dirs:
    for path, dirs, files in os.walk(directory):
      for filename in files:
        for key in extensions.keys():
          if extensions[key].match(filename):
            file_list[key].append(os.path.join(path, filename))
            break

def main():
  (options, args) = option_parser.parse_args()

  print "Crawling source files."
  find_source_files(["."])

  for language in file_list.keys():
    if len(file_list[language]) > 0:
      out_file = open(language + "_files", "w")
      out_file.write(" ".join(file_list[language]))
      out_file.close()

  etags_commands = {
    'TAGS' : "cat cpp_files | xargs " + options.etags + " -o TAGS -a ",
    'JTAGS' : "cat java_files | xargs " + options.etags + " -o JTAGS -a ",
    'PTAGS' : "cat python_files | xargs " + options.etags + " -o PTAGS -a ",
    'TAGSC' : "cat cpp_files | xargs " + options.rtags +
              " --output=TAGSC --append ",
    'JTAGSC' : "cat java_files | xargs " + options.rtags +
               " --output=JTAGSC --append ",
    'PTAGSC' : "cat python_files | xargs " + options.rtags +
               " --output=PTAGSC --append ",
    }

  for target in etags_commands.keys():
    if len(file_list[target_to_language[target]]) > 0:
      print 'generating tag file: ' + target
      os.system('rm -f %s' % target)
      os.system(etags_commands[target])
    else:
      print 'No %s files found. Skipping %s generation.' \
        % (target_to_language[target], target)

  for target in etags_to_tags_commands.keys():
    [ infile, language, callers ] = etags_to_tags_commands[target]
    if len(file_list[target_to_language[infile]]) > 0:
      if callers:
        callers_string = "--callers"
      else:
        callers_string = "--nocallers"
      print 'translating tag file: ' + target
      os.system('rm -f %s' % target)
      command = (options.etags_to_tags + " --language=%s %s --corpus_name=%s --input_file=%s --output_file=- --base_path=%s | gzip > %s.gz") % (language, callers_string, target, infile, options.base_path, target)
      print command
      os.popen(command)
    else:
      print "No %s files found. Skipping %s translation." \
          % (target_to_language[infile], target)

if __name__ == '__main__':
  main()
