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
Unittest for etags_to_tags.py.

We test some of the individual functions in etags_to_tags to check
that they detect includes correctly.

We also have a sample etags file which we convert to a new-style tags
file and compare to an expected file. This test references a fake file
root containing a .h file, used to verify that etags_to_tags can
correctly open the original source files and detect #include lines.
"""

__author__ = 'psung@google.com (Phil Sung)'

import os
import etags_to_tags
import re
import unittest

class EtagsToTagsUnitTest(unittest.TestCase):
  def assertFilesEqual(self, expect_file, actual_file):
    """
    Utility function to check that two file objects are equal. We use
    this to compare etags_to_tags output with an expected output file.
    """
    while 1:
      expect = expect_file.readline()
      actual = actual_file.readline()
      # Detect when the 'expected' file contains a timestamp line
      # (timestamp ...) and check that the corresponding 'actual' line
      # matches the same pattern, although it will have a different
      # timestamp.
      m = re.match("\(timestamp [0-9]+\)", expect.rstrip())
      if m:
        self.assert_(re.match("\(timestamp [0-9]+\)", actual.rstrip()))
      else:
        # Otherwise, just check that the lines are equal
        self.assertEqual(expect, actual)
      if (not expect) and (not actual):
        # Quit when both files are done
        break
      elif (not expect) != (not actual):
        # Fail if one file ends before the other does
        self.fail()

  def testGenerateTagscFile(self):
    """
    Does a conversion from an etags file to a TAGSC and checks the
    output against the expected result.
    """
    filein = open("testdata/test_etags", "r")
    fileout = open("generated_TAGSC", "w")
    etags_to_tags.write_tags_file(
      filein,
      fileout,
      "c++",  # language
      "testdata/tags_file_root",  # base_path
      True,  # callers
      "cppcallers")
    filein.close()
    fileout.close()

    test_output = open("generated_TAGSC", "r")
    expected_output = open(
      "testdata/test_generated_TAGSC",
      "r")
    self.assertFilesEqual(test_output, expected_output)
    test_output.close()
    expected_output.close()

if __name__ == '__main__':
  unittest.main()
