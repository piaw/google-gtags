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
 

#include "sexpression.h"
#include "test_incl.h"

BOOST_AUTO_UNIT_TEST(SExpressionTest_Nil) {
  SExpressionNil nil;

  BOOST_CHECK_FALSE(nil.IsPair());
  BOOST_CHECK(nil.IsNil());
  BOOST_CHECK(nil.IsList());
  BOOST_CHECK(nil.IsAtom());
  BOOST_CHECK(nil.IsSymbol());
  BOOST_CHECK_FALSE(nil.IsString());
  BOOST_CHECK_FALSE(nil.IsInteger());
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_Pair) {
  SExpression * pair1 =
      new SExpressionPair(new SExpressionNil(),
                          new SExpressionPair(new SExpressionNil(),
                                              new SExpressionNil()));
  BOOST_CHECK_EQUAL("(nil nil)", pair1->Repr());
  BOOST_CHECK(pair1->IsPair());
  BOOST_CHECK_FALSE(pair1->IsNil());
  BOOST_CHECK(pair1->IsList());
  BOOST_CHECK_FALSE(pair1->IsAtom());
  BOOST_CHECK_FALSE(pair1->IsSymbol());
  BOOST_CHECK_FALSE(pair1->IsString());
  BOOST_CHECK_FALSE(pair1->IsInteger());

  SExpressionPair * pair2 =
      new SExpressionPair(new SExpressionNil(),
                          new SExpressionPair(new SExpressionInteger(3),
                                              new SExpressionSymbol("foo")));
  BOOST_CHECK_EQUAL("(nil 3 . foo)", pair2->Repr());
  BOOST_CHECK(pair2->IsPair());
  BOOST_CHECK_FALSE(pair2->IsNil());
  BOOST_CHECK_FALSE(pair2->IsList());
  BOOST_CHECK_FALSE(pair2->IsAtom());
  BOOST_CHECK_FALSE(pair2->IsSymbol());
  BOOST_CHECK_FALSE(pair2->IsString());
  BOOST_CHECK_FALSE(pair2->IsInteger());
  BOOST_CHECK_EQUAL(pair2->cdr()->Repr(), "(3 . foo)");

  delete pair1;
  delete pair2;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_Symbol) {
  SExpressionSymbol * symbol1 = new SExpressionSymbol("symbol-name");
  BOOST_CHECK_FALSE(symbol1->IsPair());
  BOOST_CHECK_FALSE(symbol1->IsNil());
  BOOST_CHECK_FALSE(symbol1->IsList());
  BOOST_CHECK(symbol1->IsAtom());
  BOOST_CHECK(symbol1->IsSymbol());
  BOOST_CHECK_FALSE(symbol1->IsString());
  BOOST_CHECK_FALSE(symbol1->IsInteger());
  BOOST_CHECK_EQUAL("symbol-name", symbol1->name());
  BOOST_CHECK_EQUAL("symbol-name", symbol1->Repr());

  SExpressionSymbol * symbol2 =
      new SExpressionSymbol("symbol-with-\"quote");
  BOOST_CHECK_EQUAL("symbol-with-\"quote", symbol2->name());
  BOOST_CHECK_EQUAL("|symbol-with-\"quote|", symbol2->Repr());

  SExpressionSymbol * symbol3 = new SExpressionSymbol("symbol with spaces");
  BOOST_CHECK_EQUAL("symbol with spaces", symbol3->name());
  BOOST_CHECK_EQUAL("|symbol with spaces|", symbol3->Repr());


  // Looks like an integer; bar notation needed
  SExpressionSymbol * symbol4 = new SExpressionSymbol("505");
  BOOST_CHECK_EQUAL("|505|", symbol4->Repr());

  // Looks like an integer
  SExpressionSymbol * symbol5 = new SExpressionSymbol("-505");
  BOOST_CHECK_EQUAL("|-505|", symbol5->Repr());

  // Doesn't look like an integer, although it starts with a '-'
  SExpressionSymbol * symbol6 = new SExpressionSymbol("-80+");
  BOOST_CHECK_EQUAL("-80+", symbol6->Repr());

  // Symbol containing only periods cannot be printed literally
  SExpressionSymbol * symbol7 = new SExpressionSymbol("...");
  BOOST_CHECK_EQUAL("|...|", symbol7->Repr());

  delete symbol1;
  delete symbol2;
  delete symbol3;
  delete symbol4;
  delete symbol5;
  delete symbol6;
  delete symbol7;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_String) {
  SExpressionString * str1 = new SExpressionString("string-value");
  BOOST_CHECK_FALSE(str1->IsPair());
  BOOST_CHECK_FALSE(str1->IsNil());
  BOOST_CHECK_FALSE(str1->IsList());
  BOOST_CHECK(str1->IsAtom());
  BOOST_CHECK_FALSE(str1->IsSymbol());
  BOOST_CHECK(str1->IsString());
  BOOST_CHECK_FALSE(str1->IsInteger());
  BOOST_CHECK_EQUAL("string-value", str1->value());
  BOOST_CHECK_EQUAL("\"string-value\"", str1->Repr());

  SExpressionString * str2 = new SExpressionString("string-with-\"quotes\"");
  BOOST_CHECK_EQUAL("string-with-\"quotes\"", str2->value());
  BOOST_CHECK_EQUAL("\"string-with-\\\"quotes\\\"\"", str2->Repr());
  delete str1;
  delete str2;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_Integer) {
  SExpressionInteger * integer = new SExpressionInteger(401);
  BOOST_CHECK_FALSE(integer->IsPair());
  BOOST_CHECK_FALSE(integer->IsNil());
  BOOST_CHECK_FALSE(integer->IsList());
  BOOST_CHECK(integer->IsAtom());
  BOOST_CHECK_FALSE(integer->IsSymbol());
  BOOST_CHECK_FALSE(integer->IsString());
  BOOST_CHECK(integer->IsInteger());
  BOOST_CHECK_EQUAL(401, integer->value());
  BOOST_CHECK_EQUAL("401", integer->Repr());
  delete integer;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_ParseNil) {
  SExpression * nil1 = SExpression::Parse("()");
  BOOST_CHECK_EQUAL("nil", nil1->Repr());
  BOOST_CHECK_FALSE(nil1->IsPair());
  BOOST_CHECK(nil1->IsNil());
  BOOST_CHECK(nil1->IsList());
  BOOST_CHECK(nil1->IsAtom());
  BOOST_CHECK(nil1->IsSymbol());
  BOOST_CHECK_FALSE(nil1->IsString());
  BOOST_CHECK_FALSE(nil1->IsInteger());

  SExpression * nil2 = SExpression::Parse("(  )");
  BOOST_CHECK_EQUAL("nil", nil2->Repr());
  BOOST_CHECK(nil2->IsNil());

  SExpression * nil3 = SExpression::Parse(" nil ");
  BOOST_CHECK_EQUAL("nil", nil3->Repr());
  BOOST_CHECK(nil3->IsNil());

  delete nil1;
  delete nil2;
  delete nil3;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_ParseInteger) {
  SExpression * integer1 = SExpression::Parse("  4010");
  BOOST_CHECK_EQUAL("4010", integer1->Repr());
  BOOST_CHECK(integer1->IsAtom());

  SExpression * integer2 = SExpression::Parse("-4011  ");
  BOOST_CHECK_EQUAL("-4011", integer2->Repr());

  SExpression * integer3 = SExpression::Parse("0");
  BOOST_CHECK_EQUAL("0", integer3->Repr());

  delete integer1;
  delete integer2;
  delete integer3;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_ParseSymbol) {
  SExpression * symbol1 = SExpression::Parse("  symbol-name");
  BOOST_CHECK_EQUAL("symbol-name", symbol1->Repr());

  SExpression * symbol2 = SExpression::Parse("*symbol-name*  ");
  BOOST_CHECK_EQUAL("*symbol-name*", symbol2->Repr());

  // Name which needs bars
  SExpression * symbol3 = SExpression::Parse("|name in bars|");
  BOOST_CHECK_EQUAL("|name in bars|", symbol3->Repr());
  BOOST_CHECK_EQUAL("name in bars",
            (down_cast<SExpressionSymbol*>(symbol3))->name());

  // Name which contains bars
  SExpression * symbol4 = SExpression::Parse("|\\|\\||");
  BOOST_CHECK_EQUAL("|\\|\\||", symbol4->Repr());
  BOOST_CHECK_EQUAL("||",
            (down_cast<SExpressionSymbol*>(symbol4))->name());

  // Looks like an integer, but has an escaped char
  SExpression * symbol5 = SExpression::Parse("\\500");
  BOOST_CHECK_EQUAL("|500|", symbol5->Repr());

  delete symbol1;
  delete symbol2;
  delete symbol3;
  delete symbol4;
  delete symbol5;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_ParseString) {
  SExpression * str1 = SExpression::Parse("  \"string-value\"");
  BOOST_CHECK_EQUAL("\"string-value\"", str1->Repr());

  SExpression * str2 = SExpression::Parse("\"string-value\"  ");
  BOOST_CHECK_EQUAL("\"string-value\"", str2->Repr());

  delete str1;
  delete str2;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_ParseList) {
  SExpression * list1 = SExpression::Parse("(  one  two three  )");
  BOOST_CHECK_EQUAL("(one two three)", list1->Repr());

  SExpression * list2 = SExpression::Parse("(401)");
  BOOST_CHECK_EQUAL("(401)", list2->Repr());

  delete list1;
  delete list2;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_ParsePair) {
  SExpression * pair = SExpression::Parse("(  one  .  two  )");
  BOOST_CHECK_EQUAL("two",
            (down_cast<SExpressionPair*>(pair))->cdr()->Repr());
  BOOST_CHECK_EQUAL("(one . two)", pair->Repr());

  SExpression * lst = SExpression::Parse("(3 . (1 . (4)))");
  BOOST_CHECK_EQUAL("(3 1 4)", lst->Repr());

  SExpression * improper_list = SExpression::Parse("(a a . (b . c))");
  BOOST_CHECK_EQUAL("(a a b . c)", improper_list->Repr());

  delete pair;
  delete lst;
  delete improper_list;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_Iterator) {
  SExpression * list1 = SExpression::Parse("(1 3 5)");
  SExpression::const_iterator iter1 = list1->Begin();
  BOOST_CHECK_FALSE(iter1 == list1->End());
  BOOST_CHECK_EQUAL("1", iter1->Repr());
  ++iter1;
  BOOST_CHECK_FALSE(iter1 == list1->End());
  BOOST_CHECK_EQUAL("3", iter1->Repr());
  ++iter1;
  BOOST_CHECK_FALSE(iter1 == list1->End());
  BOOST_CHECK_EQUAL("5", iter1->Repr());
  ++iter1;
  BOOST_CHECK(iter1 == list1->End());

  SExpression * list2 = SExpression::Parse("()");
  SExpression::const_iterator iter2 = list2->Begin();
  BOOST_CHECK(iter2 == list2->End());

  delete list1;
  delete list2;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_IncompleteSexp) {
  SExpression * incomplete1 = SExpression::Parse("((())()");
  BOOST_CHECK(incomplete1 == NULL);

  SExpression * incomplete2 =
      SExpression::Parse("\"incomplete string");
  BOOST_CHECK(incomplete2 == NULL);

  SExpression * incomplete3 =
      SExpression::Parse("\"incomplete string\\");
  BOOST_CHECK(incomplete3 == NULL);

  SExpression * incomplete4 =
      SExpression::Parse("((()) (\"incomplete string");
  BOOST_CHECK(incomplete4 == NULL);

  SExpression * incomplete5 = SExpression::Parse("   ");
  BOOST_CHECK(incomplete5 == NULL);

  // A token of all periods may not be interpreted as a symbol
  SExpression * incomplete6 = SExpression::Parse(" .  ");
  BOOST_CHECK(incomplete6 == NULL);

  SExpression * incomplete7 = SExpression::Parse(" ....  ");
  BOOST_CHECK(incomplete7 == NULL);

  delete incomplete1;
  delete incomplete2;
  delete incomplete3;
  delete incomplete4;
  delete incomplete5;
  delete incomplete6;
  delete incomplete7;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_FileReader) {
  SExpression::FileReader f(
      GET_FLAG(test_srcdir) + "/testdata/test_sexpressions");

  BOOST_CHECK(!f.IsDone());
  SExpression * s1 = f.GetNext();
  BOOST_CHECK_EQUAL("symbol", s1->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s2 = f.GetNext();
  BOOST_CHECK_EQUAL("(simple list)", s2->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s3 = f.GetNext();
  BOOST_CHECK_EQUAL("(list spanning 3 lines)", s3->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s4 = f.GetNext();
  BOOST_CHECK_EQUAL("multiple-items", s4->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s5 = f.GetNext();
  BOOST_CHECK_EQUAL("on-one-line", s5->Repr());
  BOOST_CHECK(f.IsDone());

  delete s1;
  delete s2;
  delete s3;
  delete s4;
  delete s5;
}

BOOST_AUTO_UNIT_TEST(SExpressionTest_GZippedFileReader) {
  // Copy the input file from the previous test and gzip it
  string cp_command;
  cp_command.append("cp ");
  cp_command.append(GET_FLAG(test_srcdir));
  cp_command.append("/testdata/test_sexpressions ");
  cp_command.append(GET_FLAG(test_tmpdir));
  cp_command.append("/test_sexpressions");
  LOG(INFO) << "Invoking: " << cp_command;
  system(cp_command.c_str());

  string gzip_command;
  gzip_command.append("gzip -f ");
  gzip_command.append(GET_FLAG(test_tmpdir));
  gzip_command.append("/test_sexpressions");
  LOG(INFO) << "Invoking: " << gzip_command;
  system(gzip_command.c_str());

  SExpression::FileReader f(
      GET_FLAG(test_tmpdir) + "/test_sexpressions.gz",
      true);

  BOOST_CHECK(!f.IsDone());
  SExpression * s1 = f.GetNext();
  BOOST_CHECK_EQUAL("symbol", s1->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s2 = f.GetNext();
  BOOST_CHECK_EQUAL("(simple list)", s2->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s3 = f.GetNext();
  BOOST_CHECK_EQUAL("(list spanning 3 lines)", s3->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s4 = f.GetNext();
  BOOST_CHECK_EQUAL("multiple-items", s4->Repr());
  BOOST_CHECK(!f.IsDone());
  SExpression * s5 = f.GetNext();
  BOOST_CHECK_EQUAL("on-one-line", s5->Repr());
  BOOST_CHECK(f.IsDone());

  delete s1;
  delete s2;
  delete s3;
  delete s4;
  delete s5;
}
