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
#include "indexagent.h"

#include "file.h"
#include "pcqueue.h"
#include "tagsoptionparser.h"

DECLARE_STRING(gentags_local);
DECLARE_BOOL(index_callgraph);
DECLARE_INT32(index_pending_timer);

using gtags::IndexAgent;
using gtags::File;

namespace {

TEST(IndexAgentTest, GetRequests) {
  GET_FLAG(index_pending_timer) = 0;

  FilenamePCQueue q(10);
  const char* f1 = "f1";
  const char* f2 = "f2";
  const char* f3 = "f3";
  q.Put(const_cast<char*>(f1));
  q.Put(const_cast<char*>(f2));
  q.Put(const_cast<char*>(f3));

  IndexAgent index_agent(&q, NULL, NULL);
  list<const char*> file_list;
  index_agent.GetRequests(&file_list);

  EXPECT_EQ(3, file_list.size());
  list<const char*>::iterator i = file_list.begin();
  EXPECT_EQ(f1, *i);
  ++i;
  EXPECT_EQ(f2, *i);
  ++i;
  EXPECT_EQ(f3, *i);
  file_list.clear();

  q.Put(const_cast<char*>(f3));
  index_agent.GetRequests(&file_list);
  EXPECT_EQ(1, file_list.size());
  EXPECT_EQ(f3, *(file_list.begin()));
}

TEST(IndexAgentTest, DoneRequests) {
  list<const char*> file_list;

  const char* f1 = new char[20];
  const char* f2 = new char[20];
  file_list.push_back(f1);
  file_list.push_back(f2);

  string def_file = "file1";
  string caller_file = "file2";

  IndexAgent index_agent(NULL, NULL, NULL);
  index_agent.DoneRequests(&file_list, &def_file, &caller_file);

  EXPECT_EQ(0, file_list.size());
  EXPECT_EQ(0, def_file.size());
  EXPECT_EQ(0, caller_file.size());
}

TEST(IndexAgentTest, Index) {
  // We don't actually check the content of the generated files, that's
  // local_gentags_test's job.  We only ensure that Index generates tags files.
  // In fact, local_gentags won't even load any content into the tags files when
  // the automated test daemon runs because the specified files don't exist.
  // That's OK because we're only verifying that Index is generating files and
  // outputting strings that point to those files.

  list<const char*> file_list;
  file_list.push_back("file1");
  file_list.push_back("file2");

  string def_file;
  string callgraph_file;

  // To ensure we're using up-to-date binaries, we must direct IndexAgent
  // to the built binaries
  GET_FLAG(gentags_local) =
      GET_FLAG(test_srcdir) + "/local_gentags.py";
  GET_FLAG(index_callgraph) = true;

  IndexAgent index_agent(NULL, NULL, NULL);
  index_agent.Index(file_list, &def_file, &callgraph_file);

  EXPECT_STRNE(def_file.c_str(), "");
  EXPECT_STRNE(callgraph_file.c_str(), "");

  EXPECT_TRUE(File::Exists(def_file));
  EXPECT_TRUE(File::Exists(callgraph_file));

  EXPECT_TRUE(File::Delete(def_file));
  EXPECT_TRUE(File::Delete(callgraph_file));
}

}  // namespace
