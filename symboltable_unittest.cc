// Copyright 2006 Google Inc. All Rights Reserved.
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
// Author: psung@google.com (Phil Sung)
//
// Note: This file is branched from unittest of the same name
// in google3/tools/tags. Unless a change is specific for the
// open source version of the unittest, please make changes to
// the file in google3/tools/tags and downintegrate.


#include "symboltable.h"
#include "test_incl.h"

BOOST_AUTO_UNIT_TEST(SymbolTableTest_Get) {
  SymbolTable* t = new SymbolTable();
  string s1("first string");
  string s2("first string");
  string s3("second string");

  // This ought to be true if we are to be confident that we are
  // actually eliminating duplication of strings in the SymbolTable
  BOOST_CHECK_NOT_EQUAL(s1.c_str(), s2.c_str());

  const char* c1 = t->Get(s1.c_str());
  const char* c2 = t->Get(s2.c_str());
  const char* c3 = t->Get(s3.c_str());

  // It's important that we check pointer equality (not string
  // equality) here
  BOOST_CHECK_EQUAL(c1, c2);

  BOOST_CHECK_EQUAL(s1, c1);
  BOOST_CHECK_EQUAL(s2, c2);
  BOOST_CHECK_EQUAL(s3, c3);

  delete t;
}
