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

#include "gtagsunit.h"
#include "symboltable.h"

namespace {

TEST(SymbolTableTest, Get) {
  SymbolTable* t = new SymbolTable();
  string s1("first string");
  string s2("first string");
  string s3("second string");

  // This ought to be true if we are to be confident that we are
  // actually eliminating duplication of strings in the SymbolTable
  EXPECT_NE(s1.c_str(), s2.c_str());

  const char* c1 = t->Get(s1.c_str());
  const char* c2 = t->Get(s2.c_str());
  const char* c3 = t->Get(s3.c_str());

  // It's important that we check pointer equality (not string
  // equality) here
  EXPECT_EQ(c1, c2);

  EXPECT_EQ(s1, c1);
  EXPECT_EQ(s2, c2);
  EXPECT_EQ(s3, c3);

  delete t;
}

}  // namespace
