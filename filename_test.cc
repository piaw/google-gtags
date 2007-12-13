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
#include "filename.h"

#include "symboltable.h"

namespace {

TEST(FilenameTest, SymbolTableConstructor) {
  SymbolTable table;
  Filename f1("tools/tags/file.cc", &table);
  EXPECT_EQ("tools/tags/file.cc", f1.Str());
  Filename f2("tools/tags/", &table);
  EXPECT_EQ("tools/tags/", f2.Str());
}

TEST(FilenameTest, SymbolTableCopyConstructor) {
  SymbolTable table;
  // Check that copy constructor works and that data is actually
  // copied, not aliased
  Filename* f1(new Filename("tools/tags/file.cc", &table));
  Filename* f2(new Filename(*f1));
  delete f1;
  EXPECT_EQ("tools/tags/file.cc", f2->Str());
  delete f2;
}

TEST(FilenameTest, NoSymbolTableConstructor) {
  Filename f1("tools/tags/file.cc");
  EXPECT_EQ("tools/tags/file.cc", f1.Str());
  Filename f2("tools/tags/");
  EXPECT_EQ("tools/tags/", f2.Str());
}

TEST(FilenameTest, NoSymbolTableCopyConstructor) {
  // Check that copy constructor works and that data is actually
  // copied, not aliased
  Filename* f1(new Filename("tools/tags/file.cc"));
  Filename* f2(new Filename(*f1));
  delete f1;
  EXPECT_EQ("tools/tags/file.cc", f2->Str());
  delete f2;
}

TEST(FilenameTest, RemoveDotDirectories) {
  SymbolTable table;
  Filename f1("./tools/tags/./file.cc", &table);

  EXPECT_EQ("tools/tags/file.cc", f1.Str());
}

TEST(FilenameTest, RootDirectory) {
  SymbolTable table;
  Filename f1(".", &table);
  EXPECT_EQ(".", f1.Str());
  Filename f2("./.", &table);
  EXPECT_EQ(".", f1.Str());
}

TEST(FilenameTest, DirectoryDistance) {
  SymbolTable table;
  Filename f1(".", &table);
  Filename f2("a1/a2/b1/b2/b3/file.cc", &table);
  Filename f3("a1/a2/c1/c2/c3/c4/file.cc", &table);

  EXPECT_EQ(5, f2.DistanceTo(f1));
  EXPECT_EQ(5, f1.DistanceTo(f2));
  EXPECT_EQ(7, f2.DistanceTo(f3));
  EXPECT_EQ(7, f3.DistanceTo(f2));
}

TEST(FilenameTest, LessthanOperator) {
  SymbolTable table;
  Filename f1("tools/tags/file.cc", &table);
  Filename f2("tools/tags/file.h", &table);
  EXPECT_TRUE(f1 < f2);
  EXPECT_FALSE(f2 < f1);

  Filename f3("tools/tags/file.c");
  Filename f4("tools/tags/file.cc");
  EXPECT_TRUE(f3 < f4);
  EXPECT_FALSE(f4 < f3);

  Filename f5("tools/tags/file.cc", &table);
  Filename f6("tools/tags/file.h");
  EXPECT_TRUE(f5 < f6);
  EXPECT_FALSE(f6 < f5);
}

TEST(FilenameTest, EqualityOperator) {
  SymbolTable table;
  Filename f1("tools/tags/file.cc", &table);
  Filename f2("tools/tags/file.cc");
  Filename f3("tools/tags/file.h", &table);

  // f1 ought to not equal f2 for our test to be meaningful
  EXPECT_NE(&f1, &f2);

  EXPECT_TRUE(f1 == f1);
  EXPECT_TRUE(f1 == f2);

  EXPECT_TRUE(f1 != f3);
  EXPECT_TRUE(f2 != f3);
}

TEST(FilenameTest, Basename) {
  SymbolTable table;

  Filename f1("tools/tags/file.cc", &table);
  EXPECT_STREQ(f1.Basename(), "file.cc");
  Filename f2("tools/tags/", &table);
  EXPECT_STREQ(f2.Basename(), "tags");
  Filename f3(".");
  EXPECT_EQ(f3.Basename(), static_cast<const char*>(NULL));
  Filename f4("////", &table);
  EXPECT_EQ(f4.Basename(), static_cast<const char*>(NULL));
}

}  // namespace
