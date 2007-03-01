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

#include <list>
#include "test_incl.h"
#include "tagstable.h"

namespace Flag_Name_findfile {
extern tools_tags_tagsoptionparser::BooleanOptions * registry;
}

struct Fixture {
  Fixture() {
    // Turn findfile on so we can test it
    GET_FLAG(findfile) = true;

    tags_table = new TagsTable(true);

    tags_table->ReloadTagFile(
        GET_FLAG(test_srcdir) + "/testdata/test_TAGS",
        false);
  }

  ~Fixture() {
    delete tags_table;
  }

  TagsTable* tags_table;
};

BOOST_AUTO_UNIT_TEST(TagsTableTest_CallersByDefault) {
  Fixture f;
  BOOST_CHECK_FALSE(f.tags_table->SearchCallersByDefault());
}

BOOST_AUTO_UNIT_TEST(TagsTableTest_Regexp) {
  Fixture f;
  // We use static_cast<string>(...).c_str() throughout to force the
  // allocation of new strings, to make sure that we're doing string
  // comparison (not pointer comparison) in lookups.

  list<const TagsTable::TagsResult*> *
    results1 = f.tags_table->FindRegexpTags(static_cast<string>("Tags").c_str(),
                                        "",
                                        false);
  // Should contain:
  //   file3.h : class TagsReader {
  BOOST_CHECK_EQUAL(1, results1->size());

  list<const TagsTable::TagsResult*> *
    results2 =f.tags_table->FindRegexpTags(static_cast<string>("file").c_str(),
                                        "",
                                        false);
  // Should contain:
  //   file1.h : int file_size;
  //   file1.h : string file_name;
  //   file2.h : string file_name;
  BOOST_CHECK_EQUAL(3, results2->size());

  delete results1;
  delete results2;
}

BOOST_AUTO_UNIT_TEST(TagsTableTest_Snippet) {
  Fixture f;
  list<const TagsTable::TagsResult*> *
    results1 = f.tags_table->FindSnippetMatches(
        static_cast<string>("Tags").c_str(), "", false);
  // Should contain:
  //   file3.h : class TagsReader {
  //   file3.h : class BetterTagsReader : public TagsReader {
  BOOST_CHECK_EQUAL(2, results1->size());

  list<const TagsTable::TagsResult*> *
    results2 = f.tags_table->FindSnippetMatches(static_cast<string>(";").c_str(),
                                              "",
                                              false);
  // Should contain:
  //   file1.h : int file_size;
  //   file1.h : string file_name;
  //   file2.h : string file_name;
  BOOST_CHECK_EQUAL(3, results2->size());

  delete results1;
  delete results2;
}

BOOST_AUTO_UNIT_TEST(TagsTableTest_Matching) {
  Fixture f;
  list<const TagsTable::TagsResult*> *
    results1 = f.tags_table->FindTags("TagsReader",
                                    "",
                                    false);
  // Should contain:
  //   file3.h : class TagsReader {
  BOOST_CHECK_EQUAL(1, results1->size());

  list<const TagsTable::TagsResult*> *
    results2 = f.tags_table->FindTags(static_cast<string>("file_name").c_str(),
                                    "",
                                    false);
  // Should contain:
  //   file1.h : string file_name;
  //   file2.h : string file_name;
  BOOST_CHECK_EQUAL(2, results2->size());

  delete results1;
  delete results2;
}

BOOST_AUTO_UNIT_TEST(TagsTableTest_TagsInFile) {
  Fixture f;
  list<const TagsTable::TagsResult*> *
    results = f.tags_table->FindTagsByFile(
        static_cast<string>("tools/tags/file1.h").c_str(),
        false);
  // Should contain:
  //   file1.h : int file_size;
  //   file1.h : string file_name;
  BOOST_CHECK_EQUAL(2, results->size());

  delete results;
}

BOOST_AUTO_UNIT_TEST(TagsTableTest_FindFile) {
  Fixture f;
  set<string> *
    results = f.tags_table->FindFile(static_cast<string>("file2.h").c_str());
  BOOST_CHECK_EQUAL(1, results->size());
  BOOST_CHECK_EQUAL(*(results->begin()), "tools/util/file2.h");
  delete results;
}

BOOST_AUTO_UNIT_TEST(TagsTableTest_TagsResult) {
  Fixture f;
  list<const TagsTable::TagsResult*> * results =
      f.tags_table->FindSnippetMatches(static_cast<string>("TagsReader").c_str(),
                                     "tools/cpp/file4.h",
                                     false);
  // Should contain:
  //   file3.h : class TagsReader {
  //   file4.h : class BetterTagsReader : public TagsReader {
  BOOST_CHECK_EQUAL(2, results->size());

  list<const TagsTable::TagsResult*>::const_iterator iter
    = results->begin();

  TagsTable::TagsResult result1 = **iter;
  ++iter;
  TagsTable::TagsResult result2 = **iter;

  // current_file is file4.h so we should rank file4.h first

  BOOST_CHECK_STREQUAL("BetterTagsReader", result1.tag);
  BOOST_CHECK_STREQUAL("class BetterTagsReader : public Tagsreader {", result1.linerep);
  BOOST_CHECK_EQUAL("tools/cpp/file4.h", result1.filename->Str());
  BOOST_CHECK_EQUAL(30, result1.lineno);
  BOOST_CHECK_EQUAL(500, result1.charno);

  BOOST_CHECK_STREQUAL("TagsReader", result2.tag);
  BOOST_CHECK_STREQUAL("class TagsReader {", result2.linerep);
  BOOST_CHECK_EQUAL("tools/cpp/file3.h", result2.filename->Str());
  BOOST_CHECK_EQUAL(25, result2.lineno);
  BOOST_CHECK_EQUAL(400, result2.charno);

  delete results;
}
