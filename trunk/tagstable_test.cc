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

#include <list>

#include "gtagsunit.h"
#include "tagstable.h"

#include "tagsoptionparser.h"

DECLARE_BOOL(findfile);

namespace {

GTAGS_FIXTURE(TagsTableTest) {
 protected:
  GTAGS_FIXTURE_SETUP(TagsTableTest) {
    // Turn findfile on so we can test it
    GET_FLAG(findfile) = true;

    tags_table = new TagsTable(true);

    tags_table->ReloadTagFile(
        TEST_DATA_DIR + "/test_TAGS",
        false);
  }

  GTAGS_FIXTURE_TEARDOWN(TagsTableTest) {
    delete tags_table;
  }

  TagsTable* tags_table;
};

TEST_F(TagsTableTest, CallersByDefault) {
  EXPECT_FALSE(tags_table->SearchCallersByDefault());
}

TEST_F(TagsTableTest, UnloadFilesInDir) {
  // from tools/cpp/file3.h
  list<const TagsTable::TagsResult*> * results1 =
      tags_table->FindTags("TagsReader", "", false, NULL);
  EXPECT_EQ(1, results1->size());
  // from tools/cpp/file4.h
  list<const TagsTable::TagsResult*> * results2 =
      tags_table->FindTags("BetterTagsReader", "", false, NULL);
  EXPECT_EQ(1, results2->size());
  list<const TagsTable::TagsResult*> * results3 =
      tags_table->FindTags("file_name", "", false, NULL);
  EXPECT_EQ(2, results3->size());

  // Unload everthing from tools/cpp
  tags_table->UnloadFilesInDir("tools/cpp");

  // file_name is uneffected.
  list<const TagsTable::TagsResult*> * results4 =
      tags_table->FindTags("file_name", "", false, NULL);
  EXPECT_EQ(2, results4->size());

  // tags from tools/cpp are gone.
  list<const TagsTable::TagsResult*> * results5 =
      tags_table->FindTags("TagsReader", "", false, NULL);
  EXPECT_EQ(0, results5->size());
  list<const TagsTable::TagsResult*> * results6 =
      tags_table->FindTags("BetterTagsReader", "", false, NULL);
  EXPECT_EQ(0, results6->size());

  delete results1;
  delete results2;
  delete results3;
  delete results4;
  delete results5;
  delete results6;
}

TEST_F(TagsTableTest, UpdateTagfile) {
  // Before
  // Should have two file_name tag.
  // Should have one TagsReader tag.
  // Should have no file_test tag.
  list<const TagsTable::TagsResult*> *
      results1 = tags_table->FindTags("TagsReader", "", false, NULL);
  EXPECT_EQ(1, results1->size());

  list<const TagsTable::TagsResult*> *
      results2 = tags_table->FindTags("file_name", "", false, NULL);
  EXPECT_EQ(2, results2->size());

  list<const TagsTable::TagsResult*> *
      results3 = tags_table->FindTags("file_test", "", false, NULL);

  EXPECT_EQ(0, results3->size());

  tags_table->UpdateTagFile(
      TEST_DATA_DIR + "/test_update_TAGS",
      false);

  // After
  // Should have only one file_name tag.
  // should have one TagsReader tag.
  // Should have one file_test tag.
  list<const TagsTable::TagsResult*> *
      results4 = tags_table->FindTags("file_name", "", false, NULL);
  EXPECT_EQ(1, results4->size());

  list<const TagsTable::TagsResult*> *
      results5 = tags_table->FindTags("file_test", "", false, NULL);

  EXPECT_EQ(1, results5->size());

  list<const TagsTable::TagsResult*> *
      results6 = tags_table->FindTags("TagsReader", "", false, NULL);
  EXPECT_EQ(1, results6->size());

  delete results1;
  delete results2;
  delete results3;
  delete results4;
  delete results5;
  delete results6;
}

TEST_F(TagsTableTest, Regexp) {
  // We use static_cast<string>(...).c_str() throughout to force the
  // allocation of new strings, to make sure that we're doing string
  // comparison (not pointer comparison) in lookups.

  list<const TagsTable::TagsResult*> *
    results1 = tags_table->FindRegexpTags(
        static_cast<string>("Tags").c_str(), "", false, NULL);
  // Should contain:
  //   file3.h : class TagsReader {
  EXPECT_EQ(1, results1->size());

  list<const TagsTable::TagsResult*> *
    results2 = tags_table->FindRegexpTags(
        static_cast<string>("file").c_str(), "", false, NULL);
  // Should contain:
  //   file1.h : int file_size;
  //   file1.h : string file_name;
  //   file2.h : string file_name;
  EXPECT_EQ(3, results2->size());

  delete results1;
  delete results2;
}

TEST_F(TagsTableTest, Snippet) {
  list<const TagsTable::TagsResult*> *
    results1 = tags_table->FindSnippetMatches(
        static_cast<string>("Tags").c_str(), "", false, NULL);
  // Should contain:
  //   file3.h : class TagsReader {
  //   file3.h : class BetterTagsReader : public TagsReader {
  EXPECT_EQ(2, results1->size());

  list<const TagsTable::TagsResult*> *
    results2 = tags_table->FindSnippetMatches(
        static_cast<string>(";").c_str(), "", false, NULL);
  // Should contain:
  //   file1.h : int file_size;
  //   file1.h : string file_name;
  //   file2.h : string file_name;
  EXPECT_EQ(3, results2->size());

  delete results1;
  delete results2;
}

TEST_F(TagsTableTest, Matching) {
  list<const TagsTable::TagsResult*> *
      results1 = tags_table->FindTags("TagsReader", "", false, NULL);
  // Should contain:
  //   file3.h : class TagsReader {
  EXPECT_EQ(1, results1->size());

  list<const TagsTable::TagsResult*> *
      results2 = tags_table->FindTags(
          static_cast<string>("file_name").c_str(), "", false, NULL);
  // Should contain:
  //   file1.h : string file_name;
  //   file2.h : string file_name;
  EXPECT_EQ(2, results2->size());

  list<const TagsTable::TagsResult*> *
      results3 = tags_table->FindTags("file", "", false, NULL);
  // Should contain nothing
  EXPECT_EQ(0, results3->size());

  list<const TagsTable::TagsResult*> *
      results4 = tags_table->FindTags("doSomething", "", false, NULL);
  // Should contain:
  //   file4.h : vector<int> doSomething(int q, string z) {
  EXPECT_EQ(1, results4->size());

  delete results1;
  delete results2;
  delete results3;
  delete results4;
}

TEST_F(TagsTableTest, TagsInFile) {
  list<const TagsTable::TagsResult*> *
    results = tags_table->FindTagsByFile(
        static_cast<string>("tools/tags/file1.h").c_str(),
        false);
  // Should contain:
  //   file1.h : int file_size;
  //   file1.h : string file_name;
  EXPECT_EQ(2, results->size());

  delete results;
}

TEST_F(TagsTableTest, FindFile) {
  set<string> *
    results = tags_table->FindFile(static_cast<string>("file2.h").c_str());
  EXPECT_EQ(1, results->size());
  EXPECT_EQ(*(results->begin()), "tools/util/file2.h");
  delete results;
}

TEST_F(TagsTableTest, TagsResult) {
  list<const TagsTable::TagsResult*> * results =
      tags_table->FindSnippetMatches(static_cast<string>("TagsReader").c_str(),
                                     "tools/cpp/file4.h", false, NULL);
  // Should contain:
  //   file3.h : class TagsReader {
  //   file4.h : class BetterTagsReader : public TagsReader {
  EXPECT_EQ(2, results->size());

  list<const TagsTable::TagsResult*>::const_iterator iter
    = results->begin();

  TagsTable::TagsResult result1 = **iter;
  ++iter;
  TagsTable::TagsResult result2 = **iter;

  // current_file is file4.h so we should rank file4.h first

  EXPECT_STREQ("BetterTagsReader", result1.tag);
  EXPECT_STREQ("class BetterTagsReader : public Tagsreader {", result1.linerep);
  EXPECT_EQ("tools/cpp/file4.h", result1.filename->Str());
  EXPECT_EQ(30, result1.lineno);
  EXPECT_EQ(500, result1.charno);

  EXPECT_STREQ("TagsReader", result2.tag);
  EXPECT_STREQ("class TagsReader {", result2.linerep);
  EXPECT_EQ("tools/cpp/file3.h", result2.filename->Str());
  EXPECT_EQ(25, result2.lineno);
  EXPECT_EQ(400, result2.charno);

  delete results;
}

}  // namespace
