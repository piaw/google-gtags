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
#include "tagsrequesthandler.h"

#include "queryprofile.h"
#include "sexpression.h"
#include "tagsoptionparser.h"

namespace {

GTAGS_FIXTURE(SingleTableTagsRequestHandlerTest) {
 protected:
  GTAGS_FIXTURE_SETUP(SingleTableTagsRequestHandlerTest) {
    handler_ = new SingleTableTagsRequestHandler(
        TEST_DATA_DIR + "/test_TAGS",
        false, false, "google3");
  }

  GTAGS_FIXTURE_TEARDOWN(SingleTableTagsRequestHandlerTest) {
    delete handler_;
  }

  // Expects that str1 and str2 are parsed into equal s-expressions
  void ExpectSexpEq(string str1, string str2) {
    SExpression* s1 = SExpression::Parse(str1);
    SExpression* s2 = SExpression::Parse(str2);

    string str_normalized1 = s1->Repr();
    string str_normalized2 = s2->Repr();

    delete s1;
    delete s2;

    EXPECT_EQ(str_normalized1, str_normalized2);
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

  SingleTableTagsRequestHandler* handler_;
  struct query_profile log_;
  clock_t clock_;
};

TEST_F(SingleTableTagsRequestHandlerTest, OpcodePing) {
  // Ping should return 't
  ExpectSexpEq("t", handler_->Execute("#here is a comment#/", &clock_, &log_));
  EXPECT_EQ("here is a comment", log_.client);
}

TEST_F(SingleTableTagsRequestHandlerTest, OpcodeReloadFile) {
  string query;
  query.append("!");
  query.append(TEST_DATA_DIR);
  query.append("/test_empty_TAGS");

  handler_->Execute(query.c_str(), &clock_, &log_);
  ExpectSexpEq("()",
               handler_->Execute("#comment#:file_size", &clock_, &log_));
}

TEST_F(SingleTableTagsRequestHandlerTest, OpcodeLoadUpdateFile) {
  string query;
  query.append("+");
  query.append(TEST_DATA_DIR);
  query.append("/test_update_TAGS");

  handler_->Execute(query.c_str(), &clock_, &log_);

  ExpectSexpEq("((\"file_name\" \"string file_name;\" \"tools/tags/file1.h\" "
               "0 15 200) (\"file_name_1\" \"string file_name;\" "
               "\"tools/util/file2.h\" 0 30 200))",
               handler_->Execute("#comment#:file_name", &clock_, &log_));
}

TEST_F(SingleTableTagsRequestHandlerTest, OpcodeLookupPrefix) {
  ExpectSexpEq("((\"file_size\" . "
               "(\"int file_size;\" \"tools/tags/file1.h\" 0 10 100)))",
               handler_->Execute("#comment#:file_size", &clock_, &log_));
}

TEST_F(SingleTableTagsRequestHandlerTest, OpcodeLookupSnippet) {
  // This test is fragile because order of returned results is
  // unspecified
  ExpectSexpEq("((\"file_name\" . "
               "(\"string file_name;\" \"tools/tags/file1.h\" 0 15 200)) "
               "(\"file_name\" . "
               "(\"string file_name;\" \"tools/util/file2.h\" 0 20 300)))",
               handler_->Execute("#comment#$name", &clock_, &log_));
}

TEST_F(SingleTableTagsRequestHandlerTest, OpcodeLookupExact) {
  ExpectSexpEq("((\"TagsReader\" . "
               "(\"class TagsReader {\" \"tools/cpp/file3.h\" 0 25 400)))",
               handler_->Execute("#comment#;TagsReader", &clock_, &log_));
}

TEST_F(SingleTableTagsRequestHandlerTest, OpcodeLookupFile) {
  // Make a new handler which creates a file-index
  delete handler_;
  handler_ = new SingleTableTagsRequestHandler(
      TEST_DATA_DIR + "/test_TAGS",
      true, false, "google3");

  ExpectSexpEq("((\"TagsReader\" . "
               "(\"class TagsReader {\" \"tools/cpp/file3.h\" 0 25 400)))",
               handler_->Execute("#comment#@tools/cpp/file3.h",
                                &clock_,
                                &log_));
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpMalformed) {
  handler_->Execute("(ping", &clock_, &log_);
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpPing) {
  clock_ = static_cast<clock_t>(0);
  handler_->Execute("(log (client-type \"gnu-emacs\") "
                   "(client-version 1) "
                   "(protocol-version 2) (message sample-comment () 5))",
                   &clock_,
                   &log_);
  EXPECT_GT(clock_, static_cast<clock_t>(0));
  EXPECT_EQ("em", log_.client);
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpBadCommand) {
  handler_->Execute("(bad-command (client-type \"shell\") "
                   "(client-version 1) "
                   "(protocol-version 2))",
                   &clock_,
                   &log_);
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpBadClientType) {
  handler_->Execute("(bad-command (client-type \"xyz\") "
                   "(client-version 1) "
                   "(protocol-version 2))",
                   &clock_,
                   &log_);
  EXPECT_EQ("", log_.client);
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpReloadFile) {
  string query;
  query.append("(reload-tags-file (client-type \"gnu-emacs\") "
               "(client-version 1) "
               "(protocol-version 2) "
               "(file \"");
  query.append(TEST_DATA_DIR);
  query.append("/test_empty_TAGS\"))");

  handler_->Execute(query.c_str(), &clock_, &log_);
  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tag-prefix-regexp (client-type \"gnu-emacs\")"
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"file_size\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  // Server start time is always different, so just compare the rest
  // of the s-exp to expected values
  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  EXPECT_EQ(ListLength(&(*iter)), 2);
  ++iter;
  ExpectSexpEq("(sequence-number 1)", iter->Repr());
  ++iter;
  ExpectSexpEq("(value ())", iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLookupPrefix) {
  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tag-prefix-regexp (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"file_size\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  ExpectSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  ExpectSexpEq("(value (((tag \"file_size\") (snippet \"int file_size;\") "
               "(filename \"tools/tags/file1.h\") (lineno 10) "
               "(offset 100) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLookupSnippet) {
  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tag-snippet-regexp (client-type \"gnu-emacs\")"
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"name\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  ExpectSexpEq("(sequence-number 0)", iter->Repr());
  ++iter;
  // This test is fragile because order of returned results is
  // unspecified
  ExpectSexpEq("(value (((tag \"file_name\") (snippet \"string file_name;\") "
               "(filename \"tools/tags/file1.h\") (lineno 15) "
               "(offset 200) (directory-distance 0)) "
               "((tag \"file_name\") (snippet \"string file_name;\") "
               "(filename \"tools/util/file2.h\") (lineno 20) "
               "(offset 300) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLookupExact) {
  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tag-exact (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"TagsReader\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  ExpectSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  ExpectSexpEq("(value (((tag \"TagsReader\") (snippet \"class TagsReader {\") "
               "(filename \"tools/cpp/file3.h\") (lineno 25) "
               "(offset 400) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLookupFile) {
  // Make a new handler which creates a file-index
  delete handler_;
  handler_ = new SingleTableTagsRequestHandler(
      TEST_DATA_DIR + "/test_TAGS",
      true, false, "google3");

  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tags-in-file (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(file \"tools/cpp/file3.h\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  ExpectSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  ExpectSexpEq("(value (((tag \"TagsReader\") (snippet \"class TagsReader {\") "
               "(filename \"tools/cpp/file3.h\") (lineno 25) "
               "(offset 400) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLookupFileWithStripCorpus) {
  // Make a new handler which creates a file-index
  delete handler_;
  handler_ = new SingleTableTagsRequestHandler(
      TEST_DATA_DIR + "/test_TAGS",
      true, false, "google3");

  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tags-in-file (client-type \"gnu-emacs\") "
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(file \"/home/foo/google3/tools/cpp/file3.h\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  ExpectSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  ExpectSexpEq("(value (((tag \"TagsReader\") (snippet \"class TagsReader {\") "
               "(filename \"tools/cpp/file3.h\") (lineno 25) "
               "(offset 400) (directory-distance 0))))",
               iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLookupFileBadRequest) {
  // Make a new handler which creates a file-index
  delete handler_;
  handler_ = new SingleTableTagsRequestHandler(
      TEST_DATA_DIR + "/test_TAGS",
      true, false, "google3");

  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tags-in-file)", &clock_, &log_)); // No (file)

  SExpression::const_iterator iter = result->Begin();

  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  ++iter;
  ExpectSexpEq("(sequence-number 0)",
               iter->Repr());
  ++iter;
  ExpectSexpEq("(value nil)", iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST_F(SingleTableTagsRequestHandlerTest, SexpLoadUpdateFile) {
  string query;
  query.append("(load-update-file (client-type \"gnu-emacs\") "
               "(client-version 1) "
               "(protocol-version 2) "
               "(file \"");
  query.append(TEST_DATA_DIR);
  query.append("/test_update_TAGS\"))");

  handler_->Execute(query.c_str(), &clock_, &log_);
  SExpression* result = SExpression::Parse(
      handler_->Execute("(lookup-tag-exact (client-type \"gnu-emacs\")"
                       "(client-version 1) "
                       "(protocol-version 2) "
                       "(tag \"file_name\"))",
                       &clock_,
                       &log_));

  SExpression::const_iterator iter = result->Begin();

  // Server start time is always different, so just compare the rest
  // of the s-exp to expected values
  ExpectSexpEq("server-start-time",
               (down_cast<const SExpressionPair*>(&(*iter)))->car()->Repr());
  EXPECT_EQ(ListLength(&(*iter)), 2);
  ++iter;
  ExpectSexpEq("(sequence-number 1)", iter->Repr());
  ++iter;
  // We want a single file_name tag, from file1.h
  ExpectSexpEq("(value (((tag \"file_name\") (snippet \"string file_name;\") "
               "(filename \"tools/tags/file1.h\") (lineno 15) (offset 200) "
               "(directory-distance 0)))))", iter->Repr());
  ++iter;
  EXPECT_TRUE(iter == result->End());

  delete result;
}

TEST(LanguageClientTagsResultPredicateTest, Test) {
  // NOTE: Checks in this test behave very unexpectedly in this function
  //   when executed under boost.
  // EXPECT_TRUE(x) may fail where EXPECT_EQ(x, true) succeeds.
  // EXPECT_TRUE(x) may fail where bool y = x; EXPECT_TRUE(y) succeeds.

  // Expected case.
  LanguageClientTagsResultPredicate predicate("c++", "/home/usr/src/");
  TagsTable::TagsResult result;
  result.language = "c++";
  Filename f("/home/usr/src/google3/tools/tags/gtags.cc");
  result.filename = &f;

  // Boost has problems performing this check as an EXPECT_TRUE so we run it
  // as an EXPECT_EQ instead.
  EXPECT_EQ(predicate.Test(&result), true);

  // Different language.
  result.language = "java";
  EXPECT_FALSE(predicate.Test(&result));

  // Different client.
  Filename f1("/home/usr/src1/google3/tools/tags/gtags.cc");
  result.filename = &f1;
  result.language = "c++";
  EXPECT_FALSE(predicate.Test(&result));

  // Empty client. This should match any client.
  LanguageClientTagsResultPredicate predicate1("c++", "");
  EXPECT_TRUE(predicate1.Test(&result));
  result.filename = &f;
  EXPECT_TRUE(predicate1.Test(&result));
}

TEST(ProtocolRequestHandlerTest, StripCorpusRoot) {
  ProtocolRequestHandler* handler = new SexpProtocolRequestHandler(
        false, false, "google3");
  EXPECT_EQ("/path/without/corpus/root",
            handler->StripCorpusRoot("/path/without/corpus/root"));
  EXPECT_EQ("tools/tags/test.cc",
            handler->StripCorpusRoot("/path/google3/tools/tags/test.cc"));
  EXPECT_EQ("/JUNKgoogle3/test.cc",
            handler->StripCorpusRoot("/JUNKgoogle3/test.cc"));
  EXPECT_EQ("/google3JUNK/test.cc",
            handler->StripCorpusRoot("/google3JUNK/test.cc"));
  EXPECT_EQ("/JUNKgoogle3JUNK/test.cc",
            handler->StripCorpusRoot("/JUNKgoogle3JUNK/test.cc"));
  delete handler;
}

TEST(ProtocolRequestHandlerTest, StripCorpusRootNoRoot) {
  ProtocolRequestHandler* handler = new SexpProtocolRequestHandler(
        false, false, "");
  EXPECT_EQ("/path/without/corpus/root",
            handler->StripCorpusRoot("/path/without/corpus/root"));
  delete handler;
}

}  // namespace
