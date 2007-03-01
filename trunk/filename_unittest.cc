// Copyright 2007 Google Inc. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// Author: stephenchen@google.com (Stephen Chen)
//
// Note: This file is branched from unittest of the same name
// in google3/tools/tags. Unless a change is specific for the
// open source version of the unittest, please make changes to
// the file in google3/tools/tags and downintegrate. 

#include "filename.h"
#include "test_incl.h"

BOOST_AUTO_UNIT_TEST(FilenameTest_SymbolTableConstructor) {
  SymbolTable table;
  Filename f1("tools/tags/file.cc", &table);
  BOOST_CHECK_EQUAL("tools/tags/file.cc", f1.Str());
  Filename f2("tools/tags/", &table);
  BOOST_CHECK_EQUAL("tools/tags/", f2.Str());
}

BOOST_AUTO_UNIT_TEST(FilenameTest_SymbolTableCopyConstructor) {
  SymbolTable table;
  // Check that copy constructor works and that data is actually
  // copied, not aliased
  Filename* f1(new Filename("tools/tags/file.cc", &table));
  Filename* f2(new Filename(*f1));
  delete f1;
  BOOST_CHECK_EQUAL("tools/tags/file.cc", f2->Str());
  delete f2;
}

BOOST_AUTO_UNIT_TEST(FilenameTest_NoSymbolTableConstructor) {
  Filename f1("tools/tags/file.cc");
  BOOST_CHECK_EQUAL("tools/tags/file.cc", f1.Str());
  Filename f2("tools/tags/");
  BOOST_CHECK_EQUAL("tools/tags/", f2.Str());
}

BOOST_AUTO_UNIT_TEST(FilenameTest_NoSymbolTableCopyConstructor) {
  // Check that copy constructor works and that data is actually
  // copied, not aliased
  Filename* f1(new Filename("tools/tags/file.cc"));
  Filename* f2(new Filename(*f1));
  delete f1;
  BOOST_CHECK_EQUAL("tools/tags/file.cc", f2->Str());
  delete f2;
}

BOOST_AUTO_UNIT_TEST(FilenameTest_RemoveDotDirectories) {
  SymbolTable table;
  Filename f1("./tools/tags/./file.cc", &table);

  BOOST_CHECK_EQUAL("tools/tags/file.cc", f1.Str());
}

BOOST_AUTO_UNIT_TEST(FilenameTest_RootDirectory) {
  SymbolTable table;
  Filename f1(".", &table);
  BOOST_CHECK_EQUAL(".", f1.Str());
  Filename f2("./.", &table);
  BOOST_CHECK_EQUAL(".", f1.Str());
}

BOOST_AUTO_UNIT_TEST(FilenameTest_DirectoryDistance) {
  SymbolTable table;
  Filename f1(".", &table);
  Filename f2("a1/a2/b1/b2/b3/file.cc", &table);
  Filename f3("a1/a2/c1/c2/c3/c4/file.cc", &table);

  BOOST_CHECK_EQUAL(5, f2.DistanceTo(f1));
  BOOST_CHECK_EQUAL(5, f1.DistanceTo(f2));
  BOOST_CHECK_EQUAL(7, f2.DistanceTo(f3));
  BOOST_CHECK_EQUAL(7, f3.DistanceTo(f2));
}

BOOST_AUTO_UNIT_TEST(FilenameTest_LessthanOperator) {
  SymbolTable table;
  Filename f1("tools/tags/file.cc", &table);
  Filename f2("tools/tags/file.h", &table);
  BOOST_CHECK(f1 < f2);
  BOOST_CHECK_FALSE(f2 < f1);

  Filename f3("tools/tags/file.c");
  Filename f4("tools/tags/file.cc");
  BOOST_CHECK(f3 < f4);
  BOOST_CHECK_FALSE(f4 < f3);

  Filename f5("tools/tags/file.cc", &table);
  Filename f6("tools/tags/file.h");
  BOOST_CHECK(f5 < f6);
  BOOST_CHECK_FALSE(f6 < f5);
}

BOOST_AUTO_UNIT_TEST(FilenameTest_EqualityOperator) {
  SymbolTable table;
  Filename f1("tools/tags/file.cc", &table);
  Filename f2("tools/tags/file.cc");
  Filename f3("tools/tags/file.h", &table);

  // f1 ought to not equal f2 for our test to be meaningful
  BOOST_CHECK_NOT_EQUAL(&f1, &f2);

  BOOST_CHECK(f1 == f1);
  BOOST_CHECK(f1 == f2);

  BOOST_CHECK(f1 != f3);
  BOOST_CHECK(f2 != f3);
}

BOOST_AUTO_UNIT_TEST(FilenameTest_Basename) {
  SymbolTable table;

  Filename f1("tools/tags/file.cc", &table);
  BOOST_CHECK_STREQUAL(f1.Basename(), "file.cc");
  Filename f2("tools/tags/", &table);
  BOOST_CHECK_STREQUAL(f2.Basename(), "tags");
  Filename f3(".");
  BOOST_CHECK_EQUAL(f3.Basename(), static_cast<const char*>(NULL));
  Filename f4("////", &table);
  BOOST_CHECK_EQUAL(f4.Basename(), static_cast<const char*>(NULL));
}
