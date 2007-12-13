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

#include "gtagsunit.h"
#include "sexpression_util.h"
#include "sexpression_util-inl.h"

namespace {

TEST(SExpressionAssocGetTest, SExpressionAssocGet) {
  // Normal case
  SExpression* s = SExpression::Parse("((key1 value1))");
  const SExpression* result = SExpressionAssocGet(s, "key1");
  EXPECT_EQ(result->Repr(), "value1");
  delete s;

  // Multiple Items in a list
  s = SExpression::Parse("((key1 value1) (key2 value))");
  result = SExpressionAssocGet(s, "key1");
  EXPECT_EQ(result->Repr(), "value1");
  delete s;

  // Multiple keys in a list
  s = SExpression::Parse("((key1 value1) (key1 value))");
  result = SExpressionAssocGet(s, "key1");
  EXPECT_EQ(result->Repr(), "value1");
  delete s;

  // No key found in a list
  s = SExpression::Parse("((key1 value1) (key2 value))");
  result = SExpressionAssocGet(s, "key3");
  EXPECT_TRUE(result == NULL);
  delete s;
}

TEST(SExpressionAssocGetTest, SExpressionAssocReplace) {
  // Normal case
  SExpression* s = SExpression::Parse("((key1 value1))");
  string result = SExpressionAssocReplace(s, "key1", "value2");
  EXPECT_EQ("((key1 value2) )", result);
  delete s;

  // Multiple keys in a list
  s = SExpression::Parse("((key1 value1) (key1 value2))");
  result = SExpressionAssocReplace(s, "key1", "value3");
  EXPECT_EQ("((key1 value3) (key1 value3) )", result);
  delete s;

  // No key found in a list
  s = SExpression::Parse("((key1 value1) (key2 value))");
  result = SExpressionAssocReplace(s, "key3", "value3");
  EXPECT_EQ("((key1 value1) (key2 value) )", result);
  delete s;

  // Not all elements are lists
  s = SExpression::Parse("(atom (key1 value1) (key2 value))");
  result = SExpressionAssocReplace(s, "key3", "value3");
  EXPECT_EQ("(atom (key1 value1) (key2 value) )", result);
  delete s;
}

TEST(SExpressionUtilIncTest, TypeString) {
  SExpression* s = SExpression::Parse("\"some string\"");
  EXPECT_TRUE(Type<string>::IsType(s));
  EXPECT_EQ(string("some string"), Type<string>::ToType(s));
  delete s;

  s = SExpression::Parse("(not (a \"string\"))");
  EXPECT_FALSE(Type<string>::IsType(s));
  delete s;
}

TEST(SExpressionUtilIncTest, TypeBool) {
  SExpression* s = SExpression::Parse("nil");
  EXPECT_TRUE(Type<bool>::IsType(s));
  EXPECT_FALSE(Type<bool>::ToType(s));
  delete s;

  s = SExpression::Parse("(any sexpr not nil)");
  EXPECT_TRUE(Type<bool>::IsType(s));
  delete s;
}

TEST(SExpressionUtilIncTest, ReadList) {
  SExpression* s =
      SExpression::Parse("(\"token1\" \"token2\" other_type \"token3\")");
  list<string> results;
  ReadList(s->Begin(), s->End(), &results);
  EXPECT_EQ(3, results.size());
  list<string>::iterator i = results.begin();
  EXPECT_EQ(string("token1"), *i);
  ++i;
  EXPECT_EQ(string("token2"), *i);
  ++i;
  EXPECT_EQ(string("token3"), *i);
  delete s;
}

TEST(SExpressionUtilIncTest, ReadPairList) {
  SExpression* s =
      SExpression::Parse("((\"key1\" . \"value1\") sym (\"key2\" . \"value2\")"
                         "(\"key3\" . \"value3\")())");
  hash_map<string, string> results;
  ReadPairList(s->Begin(), s->End(), &results);
  EXPECT_EQ(3, results.size());
  hash_map<string, string>::iterator i = results.find("key1");
  EXPECT_EQ(string("value1"), i->second);
  i = results.find("key2");
  EXPECT_EQ(string("value2"), i->second);
  i = results.find("key3");
  EXPECT_EQ(string("value3"), i->second);
  delete s;
}

}  // namespace
