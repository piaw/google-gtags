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
// Author: nigdsouza@google.com (Nigel D'souza)

#include "gtagsunit.h"
#include "socket_filewatcher_service.h"

#include "filewatcherrequesthandler.h"
#include "socket_util.h"

namespace gtags {

TEST(SocketFileWatcherServiceTest, NoServiceTest) {
  const int port = FindAvailablePort();

  SocketFileWatcherServiceUser filewatcher_user(port);

  list<string> dirs;
  list<string> excludes;

  dirs.push_back("dir1");
  dirs.push_back("dir2");
  dirs.push_back("dir3");
  excludes.push_back("dir4");
  excludes.push_back("dir5");

  bool success;

  success = filewatcher_user.Add(dirs, excludes);
  EXPECT_FALSE(success);

  success = filewatcher_user.Remove(dirs, excludes);
  EXPECT_FALSE(success);
}

void ExpectListEq(const list<string> list1, const list<string> list2) {
  EXPECT_EQ(list1.size(), list2.size());

  for (list<string>::const_iterator iter1 = list1.begin(),
           iter2 = list2.begin();
       iter1 != list1.end(); ++iter1, ++iter2) {
    EXPECT_STREQ(iter1->c_str(), iter2->c_str());
  }
};

class MockFileWatcherRequestHandler : public FileWatcherRequestHandler {
 public:
  MockFileWatcherRequestHandler(const list<string> &expected_dirs,
                                const list<string> &expected_excludes)
      : FileWatcherRequestHandler(NULL),
        add_executed_(false), remove_executed_(false),
        expected_dirs_(expected_dirs), expected_excludes_(expected_excludes) {}

  virtual void Add(const list<string> &dirs,
                   const list<string> &excludes) {
    ExpectListEq(dirs, expected_dirs_);
    ExpectListEq(excludes, expected_excludes_);
    add_executed_ = true;
  }

  virtual void Remove(const list<string> &dirs,
                      const list<string> &excludes) {
    ExpectListEq(dirs, expected_dirs_);
    ExpectListEq(excludes, expected_excludes_);
    remove_executed_ = true;
  }

  bool add_executed_;
  bool remove_executed_;
  list<string> expected_dirs_;
  list<string> expected_excludes_;
};

TEST(SocketFileWatcherServiceTest, AddTest) {
  list<string> dirs;
  list<string> excludes;

  dirs.push_back("dir1");
  dirs.push_back("dir2");
  dirs.push_back("dir3");
  excludes.push_back("dir4");
  excludes.push_back("dir5");

  const int port = FindAvailablePort();

  MockFileWatcherRequestHandler handler(dirs, excludes);
  EXPECT_FALSE(handler.add_executed_);
  EXPECT_FALSE(handler.remove_executed_);

  SocketFileWatcherServiceProvider filewatcher_provider(port, &handler);
  filewatcher_provider.SetJoinable(true);
  filewatcher_provider.Start();

  // Busy wait until the provider has started servicing
  while (!filewatcher_provider.servicing()) {}

  SocketFileWatcherServiceUser filewatcher_user(port);
  bool success = filewatcher_user.Add(dirs, excludes);

  EXPECT_TRUE(success);
  EXPECT_FALSE(handler.remove_executed_);
  EXPECT_TRUE(handler.add_executed_);
}

TEST(SocketFileWatcherServiceTest, RemoveTest) {
  list<string> dirs;
  list<string> excludes;

  dirs.push_back("dir1");
  dirs.push_back("dir2");
  dirs.push_back("dir3");
  excludes.push_back("dir4");
  excludes.push_back("dir5");

  const int port = FindAvailablePort();

  MockFileWatcherRequestHandler handler(dirs, excludes);
  EXPECT_FALSE(handler.add_executed_);
  EXPECT_FALSE(handler.remove_executed_);

  SocketFileWatcherServiceProvider filewatcher_provider(port, &handler);
  filewatcher_provider.SetJoinable(true);
  filewatcher_provider.Start();

  // Busy wait until the provider has started servicing
  while (!filewatcher_provider.servicing()) {}

  SocketFileWatcherServiceUser filewatcher_user(port);
  bool success = filewatcher_user.Remove(dirs, excludes);

  EXPECT_TRUE(success);
  EXPECT_FALSE(handler.add_executed_);
  EXPECT_TRUE(handler.remove_executed_);
}

}  // namespace gtags
