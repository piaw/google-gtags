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
// 

#include <time.h>
#include "sexpression.h"
#include "tagsrequesthandler.h"

#include "test_incl.h"

// variables
clock_t test_clock = 0;
struct query_profile log;

// helper functions
// Asserts that str1 and str2 are parsed into equal s-expressions
void AssertSexpEq(string str1, string str2) {
  SExpression* s1 = SExpression::Parse(str1);
  SExpression* s2 = SExpression::Parse(str2);

  string str_normalized1 = s1->Repr();
  string str_normalized2 = s2->Repr();

  delete s1;
  delete s2;

  BOOST_CHECK_EQUAL(str_normalized1, str_normalized2);
}

// Returns the length of a list
int ListLength(const SExpression* s) {
  int retval = 0;
  while (!s->IsNil()) {
    s = down_cast<const SExpressionPair*>(s)->cdr();
    retval++;
  }
  return retval;
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_OpcodePing) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
                             false, false);
  // Ping should return 't
  AssertSexpEq("t", handler.Execute("#here is a comment#/", &test_clock, &log));
  BOOST_CHECK_EQUAL("here is a comment", log.client);
}


BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_OpcodeReloadFile) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
                             false, false);
  string query;
  query.append("!");
  query.append(GET_FLAG(test_srcdir));
  query.append("/testdata/test_empty_TAGS");

  handler.Execute(query.c_str(), &test_clock, &log);
  AssertSexpEq("()",
               handler.Execute("#comment#:file_size", &test_clock, &log));
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_OpcodeLookupPrefix) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
                             false, false);
  AssertSexpEq("((\"file_size\" . "
               "(\"int file_size;\" \"tools/tags/file1.h\" 0 10 100)))",
               handler.Execute("#comment#:file_size", &test_clock, &log));
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_OpcodeLookupSnippet) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
                             false, false);
  // This test is fragile because order of returned results is
  // unspecified
  AssertSexpEq("((\"file_name\" . "
               "(\"string file_name;\" \"tools/tags/file1.h\" 0 15 200)) "
               "(\"file_name\" . "
               "(\"string file_name;\" \"tools/util/file2.h\" 0 20 300)))",
               handler.Execute("#comment#$name", &test_clock, &log));
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_OpcodeLookupExact) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  AssertSexpEq("((\"TagsReader\" . "
               "(\"class TagsReader {\" \"tools/cpp/file3.h\" 0 25 400)))",
               handler.Execute("#comment#;TagsReader", &test_clock, &log));
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_OpcodeLookupFile) {
  TagsRequestHandler handler(
      GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
      true,
      false);

  AssertSexpEq("((\"TagsReader\" . "
               "(\"class TagsReader {\" \"tools/cpp/file3.h\" 0 25 400)))",
               handler.Execute("#comment#@tools/cpp/file3.h",
                                &test_clock,
                                &log));
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpMalformed) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  handler.Execute("(ping", &test_clock, &log);
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpPing) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  test_clock = static_cast<clock_t>(0);
  handler.Execute("(log (client-type \"gnu-emacs\") "
                   "(client-version 1) "
                   "(protocol-version 2) (message sample-comment () 5))",
                   &test_clock,
                   &log);
  BOOST_CHECK_EQUAL("em", log.client);
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpBadCommand) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  handler.Execute("(bad-command (client-type \"shell\") "
                   "(client-version 1) "
                   "(protocol-version 2))",
                   &test_clock,
                   &log);
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpBadClientType) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  handler.Execute("(bad-command (client-type \"xyz\") "
                   "(client-version 1) "
                   "(protocol-version 2))",
                   &test_clock,
                   &log);
  BOOST_CHECK_EQUAL("", log.client);
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpReloadFile) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  string query;
  query.append("(reload-tags-file (client-type \"gnu-emacs\") "
               "(client-version 1) "
               "(protocol-version 2) "
               "(file \"");
  query.append(GET_FLAG(test_srcdir));
  query.append("/testdata/test_empty_TAGS\"))");

  handler.Execute(query.c_str(), &test_clock, &log);
  SExpression* result = SExpression::Parse(
      handler.Execute("(lookup-tag-prefix-regexp (client-type \"gnu-emacs\")"
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"file_size\"))",
                       &test_clock,
                       &log));

  SExpression::const_iterator iter = result->Begin();

  // Server start time is always different, so just compare the rest
  // of the s-exp to expected values
  AssertSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  BOOST_CHECK_EQUAL(ListLength(&(*iter)), 2);
  ++iter;
  AssertSexpEq("(sequence-number 1)", iter->Repr());
  ++iter;
  AssertSexpEq("(value ())", iter->Repr());
  ++iter;
  BOOST_CHECK(iter == result->End());

  delete result;
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpLookupPrefix) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  SExpression* result = SExpression::Parse(
      handler.Execute("(lookup-tag-prefix-regexp (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"file_size\"))",
                       &test_clock,
                       &log));

  SExpression::const_iterator iter = result->Begin();

  AssertSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  AssertSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  AssertSexpEq("(value (((tag \"file_size\") (snippet \"int file_size;\") "
               "(filename \"tools/tags/file1.h\") (lineno 10) "
               "(offset 100) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  BOOST_CHECK(iter == result->End());

  delete result;
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpLookupSnippet) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  SExpression* result = SExpression::Parse(
      handler.Execute("(lookup-tag-snippet-regexp (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"name\"))",
                       &test_clock,
                       &log));

  SExpression::const_iterator iter = result->Begin();

  AssertSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  AssertSexpEq("(sequence-number 0)", iter->Repr());
  ++iter;
  // This test is fragile because order of returned results is
  // unspecified
  AssertSexpEq("(value (((tag \"file_name\") (snippet \"string file_name;\") "
               "(filename \"tools/tags/file1.h\") (lineno 15) "
               "(offset 200) (directory-distance 0)) "
               "((tag \"file_name\") (snippet \"string file_name;\") "
               "(filename \"tools/util/file2.h\") (lineno 20) "
               "(offset 300) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  BOOST_CHECK(iter == result->End());

  delete result;
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpLookupExact) {
  TagsRequestHandler handler(GET_FLAG(test_srcdir) + "/testdata/test_TAGS", false, false);
  SExpression* result = SExpression::Parse(
      handler.Execute("(lookup-tag-exact (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"TagsReader\"))",
                       &test_clock,
                       &log));

  SExpression::const_iterator iter = result->Begin();

  AssertSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  AssertSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  AssertSexpEq("(value (((tag \"TagsReader\") (snippet \"class TagsReader {\") "
               "(filename \"tools/cpp/file3.h\") (lineno 25) "
               "(offset 400) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  BOOST_CHECK(iter == result->End());

  delete result;
}

BOOST_AUTO_UNIT_TEST(TagsRequestHandlerTest_SexpLookupFile) {
  TagsRequestHandler handler(
      GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
      true,
      false);

  SExpression* result = SExpression::Parse(
      handler.Execute("(lookup-tags-in-file (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(file \"tools/cpp/file3.h\"))",
                       &test_clock,
                       &log));

  SExpression::const_iterator iter = result->Begin();

  AssertSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  AssertSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  AssertSexpEq("(value (((tag \"TagsReader\") (snippet \"class TagsReader {\") "
               "(filename \"tools/cpp/file3.h\") (lineno 25) "
               "(offset 400) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  BOOST_CHECK(iter == result->End());

  delete result;
}
