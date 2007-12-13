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
#include "sexpression.h"

#include "tagsoptionparser.h"

namespace {

TEST(SExpressionTest, Nil) {
  SExpressionNil nil;

  EXPECT_FALSE(nil.IsPair());
  EXPECT_TRUE(nil.IsNil());
  EXPECT_TRUE(nil.IsList());
  EXPECT_TRUE(nil.IsAtom());
  EXPECT_TRUE(nil.IsSymbol());
  EXPECT_FALSE(nil.IsString());
  EXPECT_FALSE(nil.IsInteger());
}

TEST(SExpressionTest, Pair) {
  SExpression * pair1 =
      new SExpressionPair(new SExpressionNil(),
                          new SExpressionPair(new SExpressionNil(),
                                              new SExpressionNil()));
  EXPECT_EQ("(nil nil)", pair1->Repr());
  EXPECT_TRUE(pair1->IsPair());
  EXPECT_FALSE(pair1->IsNil());
  EXPECT_TRUE(pair1->IsList());
  EXPECT_FALSE(pair1->IsAtom());
  EXPECT_FALSE(pair1->IsSymbol());
  EXPECT_FALSE(pair1->IsString());
  EXPECT_FALSE(pair1->IsInteger());

  SExpressionPair * pair2 =
      new SExpressionPair(new SExpressionNil(),
                          new SExpressionPair(new SExpressionInteger(3),
                                              new SExpressionSymbol("foo")));
  EXPECT_EQ("(nil 3 . foo)", pair2->Repr());
  EXPECT_TRUE(pair2->IsPair());
  EXPECT_FALSE(pair2->IsNil());
  EXPECT_FALSE(pair2->IsList());
  EXPECT_FALSE(pair2->IsAtom());
  EXPECT_FALSE(pair2->IsSymbol());
  EXPECT_FALSE(pair2->IsString());
  EXPECT_FALSE(pair2->IsInteger());
  EXPECT_EQ(pair2->cdr()->Repr(), "(3 . foo)");

  delete pair1;
  delete pair2;
}

TEST(SExpressionTest, Symbol) {
  SExpressionSymbol * symbol1 = new SExpressionSymbol("symbol-name");
  EXPECT_FALSE(symbol1->IsPair());
  EXPECT_FALSE(symbol1->IsNil());
  EXPECT_FALSE(symbol1->IsList());
  EXPECT_TRUE(symbol1->IsAtom());
  EXPECT_TRUE(symbol1->IsSymbol());
  EXPECT_FALSE(symbol1->IsString());
  EXPECT_FALSE(symbol1->IsInteger());
  EXPECT_EQ("symbol-name", symbol1->name());
  EXPECT_EQ("symbol-name", symbol1->Repr());

  SExpressionSymbol * symbol2 =
      new SExpressionSymbol("symbol-with-\"quote");
  EXPECT_EQ("symbol-with-\"quote", symbol2->name());
  EXPECT_EQ("|symbol-with-\"quote|", symbol2->Repr());

  SExpressionSymbol * symbol3 = new SExpressionSymbol("symbol with spaces");
  EXPECT_EQ("symbol with spaces", symbol3->name());
  EXPECT_EQ("|symbol with spaces|", symbol3->Repr());


  // Looks like an integer; bar notation needed
  SExpressionSymbol * symbol4 = new SExpressionSymbol("505");
  EXPECT_EQ("|505|", symbol4->Repr());

  // Looks like an integer
  SExpressionSymbol * symbol5 = new SExpressionSymbol("-505");
  EXPECT_EQ("|-505|", symbol5->Repr());

  // Doesn't look like an integer, although it starts with a '-'
  SExpressionSymbol * symbol6 = new SExpressionSymbol("-80+");
  EXPECT_EQ("-80+", symbol6->Repr());

  // Symbol containing only periods cannot be printed literally
  SExpressionSymbol * symbol7 = new SExpressionSymbol("...");
  EXPECT_EQ("|...|", symbol7->Repr());

  delete symbol1;
  delete symbol2;
  delete symbol3;
  delete symbol4;
  delete symbol5;
  delete symbol6;
  delete symbol7;
}

TEST(SExpressionTest, String) {
  SExpressionString * str1 = new SExpressionString("string-value");
  EXPECT_FALSE(str1->IsPair());
  EXPECT_FALSE(str1->IsNil());
  EXPECT_FALSE(str1->IsList());
  EXPECT_TRUE(str1->IsAtom());
  EXPECT_FALSE(str1->IsSymbol());
  EXPECT_TRUE(str1->IsString());
  EXPECT_FALSE(str1->IsInteger());
  EXPECT_EQ("string-value", str1->value());
  EXPECT_EQ("\"string-value\"", str1->Repr());

  SExpressionString * str2 = new SExpressionString("string-with-\"quotes\"");
  EXPECT_EQ("string-with-\"quotes\"", str2->value());
  EXPECT_EQ("\"string-with-\\\"quotes\\\"\"", str2->Repr());
  delete str1;
  delete str2;
}

TEST(SExpressionTest, Integer) {
  SExpressionInteger * integer = new SExpressionInteger(401);
  EXPECT_FALSE(integer->IsPair());
  EXPECT_FALSE(integer->IsNil());
  EXPECT_FALSE(integer->IsList());
  EXPECT_TRUE(integer->IsAtom());
  EXPECT_FALSE(integer->IsSymbol());
  EXPECT_FALSE(integer->IsString());
  EXPECT_TRUE(integer->IsInteger());
  EXPECT_EQ(401, integer->value());
  EXPECT_EQ("401", integer->Repr());
  delete integer;
}

TEST(SExpressionTest, ParseNil) {
  SExpression * nil1 = SExpression::Parse("()");
  EXPECT_EQ("nil", nil1->Repr());
  EXPECT_FALSE(nil1->IsPair());
  EXPECT_TRUE(nil1->IsNil());
  EXPECT_TRUE(nil1->IsList());
  EXPECT_TRUE(nil1->IsAtom());
  EXPECT_TRUE(nil1->IsSymbol());
  EXPECT_FALSE(nil1->IsString());
  EXPECT_FALSE(nil1->IsInteger());

  SExpression * nil2 = SExpression::Parse("(  )");
  EXPECT_EQ("nil", nil2->Repr());
  EXPECT_TRUE(nil2->IsNil());

  SExpression * nil3 = SExpression::Parse(" nil ");
  EXPECT_EQ("nil", nil3->Repr());
  EXPECT_TRUE(nil3->IsNil());

  delete nil1;
  delete nil2;
  delete nil3;
}

TEST(SExpressionTest, ParseInteger) {
  SExpression * integer1 = SExpression::Parse("  4010");
  EXPECT_EQ("4010", integer1->Repr());
  EXPECT_TRUE(integer1->IsAtom());

  SExpression * integer2 = SExpression::Parse("-4011  ");
  EXPECT_EQ("-4011", integer2->Repr());

  SExpression * integer3 = SExpression::Parse("0");
  EXPECT_EQ("0", integer3->Repr());

  delete integer1;
  delete integer2;
  delete integer3;
}

TEST(SExpressionTest, ParseSymbol) {
  SExpression * symbol1 = SExpression::Parse("  symbol-name");
  EXPECT_EQ("symbol-name", symbol1->Repr());

  SExpression * symbol2 = SExpression::Parse("*symbol-name*  ");
  EXPECT_EQ("*symbol-name*", symbol2->Repr());

  // Name which needs bars
  SExpression * symbol3 = SExpression::Parse("|name in bars|");
  EXPECT_EQ("|name in bars|", symbol3->Repr());
  EXPECT_EQ("name in bars",
            (down_cast<SExpressionSymbol*>(symbol3))->name());

  // Name which contains bars
  SExpression * symbol4 = SExpression::Parse("|\\|\\||");
  EXPECT_EQ("|\\|\\||", symbol4->Repr());
  EXPECT_EQ("||",
            (down_cast<SExpressionSymbol*>(symbol4))->name());

  // Looks like an integer, but has an escaped char
  SExpression * symbol5 = SExpression::Parse("\\500");
  EXPECT_EQ("|500|", symbol5->Repr());

  delete symbol1;
  delete symbol2;
  delete symbol3;
  delete symbol4;
  delete symbol5;
}

TEST(SExpressionTest, ParseString) {
  SExpression * str1 = SExpression::Parse("  \"string-value\"");
  EXPECT_EQ("\"string-value\"", str1->Repr());

  SExpression * str2 = SExpression::Parse("\"string-value\"  ");
  EXPECT_EQ("\"string-value\"", str2->Repr());

  delete str1;
  delete str2;
}

TEST(SExpressionTest, ParseList) {
  SExpression * list1 = SExpression::Parse("(  one  two three  )");
  EXPECT_EQ("(one two three)", list1->Repr());

  SExpression * list2 = SExpression::Parse("(401)");
  EXPECT_EQ("(401)", list2->Repr());

  delete list1;
  delete list2;
}

TEST(SExpressionTest, ParsePair) {
  SExpression * pair = SExpression::Parse("(  one  .  two  )");
  EXPECT_EQ("two",
            (down_cast<SExpressionPair*>(pair))->cdr()->Repr());
  EXPECT_EQ("(one . two)", pair->Repr());

  SExpression * lst = SExpression::Parse("(3 . (1 . (4)))");
  EXPECT_EQ("(3 1 4)", lst->Repr());

  SExpression * improper_list = SExpression::Parse("(a a . (b . c))");
  EXPECT_EQ("(a a b . c)", improper_list->Repr());

  delete pair;
  delete lst;
  delete improper_list;
}

TEST(SExpressionTest, Iterator) {
  SExpression * list1 = SExpression::Parse("(1 3 5)");
  SExpression::const_iterator iter1 = list1->Begin();
  EXPECT_FALSE(iter1 == list1->End());
  EXPECT_EQ("1", iter1->Repr());
  ++iter1;
  EXPECT_FALSE(iter1 == list1->End());
  EXPECT_EQ("3", iter1->Repr());
  ++iter1;
  EXPECT_FALSE(iter1 == list1->End());
  EXPECT_EQ("5", iter1->Repr());
  ++iter1;
  EXPECT_TRUE(iter1 == list1->End());

  SExpression * list2 = SExpression::Parse("()");
  SExpression::const_iterator iter2 = list2->Begin();
  EXPECT_TRUE(iter2 == list2->End());

  delete list1;
  delete list2;
}

TEST(SExpressionTest, IncompleteSexp) {
  SExpression * incomplete1 = SExpression::Parse("((())()");
  EXPECT_TRUE(incomplete1 == NULL);

  SExpression * incomplete2 =
      SExpression::Parse("\"incomplete string");
  EXPECT_TRUE(incomplete2 == NULL);

  SExpression * incomplete3 =
      SExpression::Parse("\"incomplete string\\");
  EXPECT_TRUE(incomplete3 == NULL);

  SExpression * incomplete4 =
      SExpression::Parse("((()) (\"incomplete string");
  EXPECT_TRUE(incomplete4 == NULL);

  SExpression * incomplete5 = SExpression::Parse("   ");
  EXPECT_TRUE(incomplete5 == NULL);

  // A token of all periods may not be interpreted as a symbol
  SExpression * incomplete6 = SExpression::Parse(" .  ");
  EXPECT_TRUE(incomplete6 == NULL);

  SExpression * incomplete7 = SExpression::Parse(" ....  ");
  EXPECT_TRUE(incomplete7 == NULL);

  delete incomplete1;
  delete incomplete2;
  delete incomplete3;
  delete incomplete4;
  delete incomplete5;
  delete incomplete6;
  delete incomplete7;
}

TEST(SExpressionTest, FileReader) {
  FileReader<SExpression> f(
      TEST_DATA_DIR + "/test_sexpressions");

  EXPECT_TRUE(!f.IsDone());
  SExpression * s1 = f.GetNext();
  EXPECT_EQ("symbol", s1->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s2 = f.GetNext();
  EXPECT_EQ("(simple list)", s2->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s3 = f.GetNext();
  EXPECT_EQ("(list spanning 3 lines)", s3->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s4 = f.GetNext();
  EXPECT_EQ("multiple-items", s4->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s5 = f.GetNext();
  EXPECT_EQ("on-one-line", s5->Repr());
  EXPECT_TRUE(f.IsDone());

  delete s1;
  delete s2;
  delete s3;
  delete s4;
  delete s5;
}

TEST(SExpressionTest, GZippedFileReader) {
  // Copy the input file from the previous test and gzip it
  string cp_command;
  cp_command.append("cp ");
  cp_command.append(TEST_DATA_DIR);
  cp_command.append("/test_sexpressions ");
  cp_command.append(GET_FLAG(test_tmpdir));
  cp_command.append("/test_sexpressions");
  LOG(INFO) << "Invoking: " << cp_command;
  system(cp_command.c_str());

  string gzip_command;
  gzip_command.append("gzip ");
  gzip_command.append(GET_FLAG(test_tmpdir));
  gzip_command.append("/test_sexpressions");
  LOG(INFO) << "Invoking: " << gzip_command;
  system(gzip_command.c_str());

  FileReader<SExpression> f(
      GET_FLAG(test_tmpdir) + "/test_sexpressions.gz",
      true);

  // Remove the gzipped file.
  string rm_gzip_command;
  rm_gzip_command.append("rm -f ");
  rm_gzip_command.append(GET_FLAG(test_tmpdir));
  rm_gzip_command.append("/test_sexpressions.gz");
  LOG(INFO) << "Invoking: " << rm_gzip_command;
  system(rm_gzip_command.c_str());

  EXPECT_TRUE(!f.IsDone());
  SExpression * s1 = f.GetNext();
  EXPECT_EQ("symbol", s1->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s2 = f.GetNext();
  EXPECT_EQ("(simple list)", s2->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s3 = f.GetNext();
  EXPECT_EQ("(list spanning 3 lines)", s3->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s4 = f.GetNext();
  EXPECT_EQ("multiple-items", s4->Repr());
  EXPECT_TRUE(!f.IsDone());
  SExpression * s5 = f.GetNext();
  EXPECT_EQ("on-one-line", s5->Repr());
  EXPECT_TRUE(f.IsDone());

  delete s1;
  delete s2;
  delete s3;
  delete s4;
  delete s5;
}

}  // namespace
