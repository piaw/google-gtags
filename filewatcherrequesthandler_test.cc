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
#include "filewatcherrequesthandler.h"

#include "filewatcher.h"
#include "pcqueue.h"
#include "tagsrequesthandler.h"

namespace gtags {

class MockProducerConsumerQueue : public WatcherCommandPCQueue {
 public:
  MockProducerConsumerQueue() : WatcherCommandPCQueue(1) {}
  virtual ~MockProducerConsumerQueue() {}

  virtual void Put(struct WatcherCommand* elem) {
    mock_queue_.push_back(elem);
  }

  virtual struct WatcherCommand* Get() {
    struct WatcherCommand* front = mock_queue_.front();
    mock_queue_.pop_front();
    return front;
  }

  deque<struct WatcherCommand*> mock_queue_;
};

TEST(FileWatcherRequestHandlerTest, Add) {
  list<string> dirs;
  list<string> excludes;

  dirs.push_back("dir1");
  dirs.push_back("dir2");
  dirs.push_back("dir3");
  excludes.push_back("dir1");
  excludes.push_back("dir2");

  MockProducerConsumerQueue q;
  gtags::FileWatcherRequestHandler handler(&q);

  handler.Add(dirs, excludes);

  struct WatcherCommand* s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir1", s->directory);
  EXPECT_EQ(gtags::EXCLUDE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir2", s->directory);
  EXPECT_EQ(gtags::EXCLUDE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir1", s->directory);
  EXPECT_EQ(gtags::ADD, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir2", s->directory);
  EXPECT_EQ(gtags::ADD, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir3", s->directory);
  EXPECT_EQ(gtags::ADD, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir1", s->directory);
  EXPECT_EQ(gtags::REMOVE_EXCLUDE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir2", s->directory);
  EXPECT_EQ(gtags::REMOVE_EXCLUDE, s->code);
  delete s;
}

TEST(FileWatcherRequestHandlerTest, Remove) {
  list<string> dirs;
  list<string> excludes;

  dirs.push_back("dir1");
  dirs.push_back("dir2");
  dirs.push_back("dir3");
  excludes.push_back("dir1");
  excludes.push_back("dir2");

  MockProducerConsumerQueue q;
  gtags::FileWatcherRequestHandler handler(&q);

  handler.Remove(dirs, excludes);

  struct WatcherCommand* s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir1", s->directory);
  EXPECT_EQ(gtags::EXCLUDE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir2", s->directory);
  EXPECT_EQ(gtags::EXCLUDE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir1", s->directory);
  EXPECT_EQ(gtags::REMOVE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir2", s->directory);
  EXPECT_EQ(gtags::REMOVE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir3", s->directory);
  EXPECT_EQ(gtags::REMOVE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir1", s->directory);
  EXPECT_EQ(gtags::REMOVE_EXCLUDE, s->code);
  delete s;

  s = static_cast<struct WatcherCommand*>(q.Get());
  EXPECT_EQ("dir2", s->directory);
  EXPECT_EQ(gtags::REMOVE_EXCLUDE, s->code);
  delete s;
}

class MockFileWatcher : public InotifyFileWatcher {
 public:
  virtual ~MockFileWatcher() {}
  virtual void AddDirectoryRecursive(const char* dir) {
    added_.push_back(dir);
  }

  virtual void RemoveDirectoryRecursive(const char* dir) {
    removed_.push_back(dir);
  }

  virtual void AddExcludeDirectory(const char* dir) {
    exclude_.push_back(dir);
  }

  virtual void RemoveExcludeDirectory(const char* dir) {
    remove_exclude_.push_back(dir);
  }

  deque<string> exclude_;
  deque<string> remove_exclude_;
  deque<string> added_;
  deque<string> removed_;
};

class MockLocalTagsRequestHandler : public LocalTagsRequestHandler {
 public:
  MockLocalTagsRequestHandler() : LocalTagsRequestHandler(
      false, false, "google3") {}
  virtual ~MockLocalTagsRequestHandler() {}

  virtual void UnloadFilesInDir(const string& dirname) {
    unloaded_.push_back(dirname);
  }

  deque<string> unloaded_;
};

TEST(FileWatcherRequestWorkerTest, Run) {
  MockProducerConsumerQueue q;
  MockLocalTagsRequestHandler def;
  MockLocalTagsRequestHandler cal;

  // These are deleted by Run.
  struct WatcherCommand* cmd0 = new struct WatcherCommand();
  struct WatcherCommand* cmd1 = new struct WatcherCommand();
  struct WatcherCommand* cmd2 = new struct WatcherCommand();
  struct WatcherCommand* cmd3 = new struct WatcherCommand();
  struct WatcherCommand* cmd4 = new struct WatcherCommand();

  cmd0->code = gtags::EXCLUDE;
  cmd0->directory = "dir1";
  cmd1->code = gtags::ADD;
  cmd1->directory = "dir1";
  cmd2->code = gtags::REMOVE;
  cmd2->directory = "dir2";
  cmd3->code = gtags::ADD;
  cmd3->directory = "dir3";
  cmd4->code = gtags::REMOVE_EXCLUDE;
  cmd4->directory = "dir1";
  q.Put(cmd0);
  q.Put(cmd1);
  q.Put(cmd2);
  q.Put(cmd3);
  q.Put(cmd4);
  q.Put(NULL);

  MockFileWatcher watcher;

  FileWatcherRequestWorker worker(&watcher, &q, &def, &cal);
  worker.SetJoinable(true);
  worker.Start();
  worker.Join();

  EXPECT_EQ("dir1", watcher.exclude_[0]);
  EXPECT_EQ("dir1", watcher.added_[0]);
  EXPECT_EQ("dir2", watcher.removed_[0]);
  EXPECT_EQ("dir2", def.unloaded_[0]);
  EXPECT_EQ("dir2", cal.unloaded_[0]);
  EXPECT_EQ("dir3", watcher.added_[1]);
  EXPECT_EQ("dir1", watcher.remove_exclude_[0]);
}

}  // namespace
