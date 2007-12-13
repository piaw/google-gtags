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
#include "filewatcher.h"

#include "pcqueue.h"
#include "tagsoptionparser.h"

namespace {

TEST(UtilityFunctions, TestJoinPath) {
  const char* childpath = "child";
  const char* parentpath = "parent";

  char buffer[kMaxPathLength];
  const char* result = JoinPath(parentpath, childpath, buffer);
  EXPECT_STREQ("parent/child", result);
}

TEST(PathnameWatchDescriptorMap, TestALL) {
  PathnameWatchDescriptorMap path_wd_map;
  // Add
  path_wd_map.Add("path1", 1);
  path_wd_map.Add("path2", 2);

  // Getters
  EXPECT_STREQ("path1", path_wd_map.GetPathname(1));
  EXPECT_STREQ("path2", path_wd_map.GetPathname(2));
  EXPECT_TRUE(path_wd_map.GetPathname(3) == NULL);

  EXPECT_EQ(1, path_wd_map.GetWatchDescriptor("path1"));
  EXPECT_EQ(2, path_wd_map.GetWatchDescriptor("path2"));
  EXPECT_EQ(0, path_wd_map.GetWatchDescriptor("path3"));

  // Remove
  path_wd_map.Remove(1);
  EXPECT_EQ(0, path_wd_map.GetWatchDescriptor("path1"));
  EXPECT_TRUE(path_wd_map.GetPathname(1) == NULL);
}

TEST(PathnameWatchDescriptorMap, TestGetSubDirs) {
  PathnameWatchDescriptorMap path_wd_map;
  const char* dir1 = "/home/build/file1";
  const char* dir2 = "/home/build/file2";
  const char* dir3 = "/home/user/file1";
  path_wd_map.Add(dir1, 1);
  path_wd_map.Add(dir2, 2);
  path_wd_map.Add(dir3, 3);

  list<const char*> results;
  path_wd_map.GetSubDirs("/home", &results);
  EXPECT_EQ(3, results.size());
  results.clear();
  path_wd_map.GetSubDirs("/home/build/", &results);
  EXPECT_EQ(2, results.size());
  results.clear();

  list<int> wd_results;
  path_wd_map.GetSubDirsWatchDescriptor("/home", &wd_results);
  EXPECT_EQ(3, wd_results.size());
  wd_results.clear();
  path_wd_map.GetSubDirsWatchDescriptor("/home/build/", &wd_results);
  EXPECT_EQ(2, wd_results.size());
  EXPECT_TRUE(find(wd_results.begin(),wd_results.end(), 1) !=
              wd_results.end());
  EXPECT_TRUE(find(wd_results.begin(), wd_results.end(), 2) !=
              wd_results.end());
  wd_results.clear();
}

GTAGS_FIXTURE(InotifyEventHandlerTest) {
 public:
  GTAGS_FIXTURE_SETUP(InotifyEventHandlerTest) {
    event_.wd = 123;
    event_.mask = 0;
  }

  void AssertInotifyEventEq(inotify_event* event1, inotify_event* event2) {
    EXPECT_EQ(event1->wd, event2->wd);
    EXPECT_EQ(event1->mask, event2->mask);
  }

  void InvokeEventAndVerify(int mask) {
    handler_.event_ = NULL;
    handler_.mask_ = 0;
    event_.mask = 0 | mask;
    handler_.HandleEvent(&event_);
    AssertInotifyEventEq(&event_, handler_.event_);
    EXPECT_EQ(mask, handler_.mask_);
    handler_.event_ = NULL;
    handler_.mask_ = 0;
  }

 private:
  class InotifyEventHandlerTester : public InotifyEventHandler {
   public:
    InotifyEventHandlerTester()
        : InotifyEventHandler(), event_(NULL), mask_(0) {}

    virtual void HandleAccess(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_ACCESS;
    }
    virtual void HandleModify(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_MODIFY;
    }
    virtual void HandleAttrib(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_ATTRIB;
    }
    virtual void HandleCloseWrite(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_CLOSE_WRITE;
    }
    virtual void HandleCloseNoWrite(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_CLOSE_NOWRITE;
    }
    virtual void HandleOpen(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_OPEN;
    }
    virtual void HandleMovedFrom(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_MOVED_FROM;
    }
    virtual void HandleMovedTo(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_MOVED_TO;
    }
    virtual void HandleCreate(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_CREATE;
    }
    virtual void HandleDelete(struct inotify_event* event) {
      event_ = event;
      mask_ = IN_DELETE;
    }

    struct inotify_event* event_;
    int mask_;
  };

 protected:
  InotifyEventHandlerTester handler_;
  struct inotify_event event_;
};

TEST_F(InotifyEventHandlerTest, TestHandlers) {
  InvokeEventAndVerify(IN_ACCESS);
  InvokeEventAndVerify(IN_MODIFY);
  InvokeEventAndVerify(IN_ATTRIB);
  InvokeEventAndVerify(IN_CLOSE_WRITE);
  InvokeEventAndVerify(IN_CLOSE_NOWRITE);
  InvokeEventAndVerify(IN_OPEN);
  InvokeEventAndVerify(IN_MOVED_FROM);
  InvokeEventAndVerify(IN_MOVED_TO);
  InvokeEventAndVerify(IN_CREATE);
  InvokeEventAndVerify(IN_DELETE);
}

struct inotify_event* CreateInotifyEvent(int wd, int mask, const char* name) {
  int length = (name == NULL) ? 0 : strlen(name);

  char* buffer = new char[sizeof(struct inotify_event) + length + 1];
  struct inotify_event* event = (struct inotify_event*) buffer;
  event->wd = wd;
  event->mask = mask;
  event->len = length;
  strncpy(event->name, name, length);
  event->name[length] = '\0';
  return event;
}

void FreeInotifyEvent(struct inotify_event* event) {
  char* buffer = (char*)event;
  delete [] buffer;
}

class MockInotifyEventHandler : public InotifyEventHandler {
 public:
  MockInotifyEventHandler() : invoked_(false) {
  }

  virtual ~MockInotifyEventHandler() {}

  virtual void HandleAccess(struct inotify_event* event) {
    invoked_ = true;
  }

  virtual void HandleImport(const char* name) {
    imported.push_back(string(name));
  }

  bool invoked() const {
    return invoked_;
  }

  void reset() {
    invoked_ = false;
  }

  bool invoked_;

  deque<string> imported;
};

TEST(DirectoryEventFilterTest, TestDoFilter) {
  MockInotifyEventHandler handler;
  DirectoryEventFilter filter;
  handler.AddFilter(&filter);

  // Test with dir bit set
  struct inotify_event* event =
      CreateInotifyEvent(1, IN_ACCESS | IN_ISDIR, "blahblah");

  // Verify handler.HandleAccess is called.
  EXPECT_FALSE(handler.invoked());
  handler.HandleEvent(event);
  EXPECT_TRUE(handler.invoked());

  // Test without dir bit set
  event->mask = IN_ACCESS;
  handler.reset();
  // Verify handler.HandleAccess is not called.
  EXPECT_FALSE(handler.invoked());
  handler.HandleEvent(event);
  EXPECT_FALSE(handler.invoked());

  FreeInotifyEvent(event);
}

TEST(FileExtensionEventFilterTest, TestDoFilter) {
  MockInotifyEventHandler handler;
  FileExtensionEventFilter filter;
  handler.AddFilter(&filter);

  filter.AddExtension(".cc");
  filter.AddExtension(".h");
  filter.AddExtension(".py");

  struct inotify_event* event =
      CreateInotifyEvent(1, IN_ACCESS | IN_ISDIR, "blah.cc");
  // Verify handler.HandleEvent is called.
  EXPECT_FALSE(handler.invoked());
  handler.HandleEvent(event);
  EXPECT_TRUE(handler.invoked());
  FreeInotifyEvent(event);

  handler.reset();
  event = CreateInotifyEvent(1, IN_ACCESS | IN_ISDIR, "blah.py");
  // Verify handler.HandleAccess is called.
  EXPECT_FALSE(handler.invoked());
  handler.HandleEvent(event);
  EXPECT_TRUE(handler.invoked());
  FreeInotifyEvent(event);

  handler.reset();
  event = CreateInotifyEvent(1, IN_ACCESS | IN_ISDIR, "blah.pyc");
  // Verify handler.HandleAccess is not called.
  EXPECT_FALSE(handler.invoked());
  handler.HandleEvent(event);
  EXPECT_FALSE(handler.invoked());
  FreeInotifyEvent(event);

  handler.reset();
  event = CreateInotifyEvent(1, IN_ACCESS | IN_ISDIR, NULL);  //NULL filename
  // Verify handler.HandleAccess is not called.
  EXPECT_FALSE(handler.invoked());
  handler.HandleEvent(event);
  EXPECT_FALSE(handler.invoked());
  FreeInotifyEvent(event);
}

class MockFilter : public InotifyEventFilter {
 public:
  MockFilter() : InotifyEventFilter() {}
  virtual ~MockFilter() {}
  virtual bool DoFilter(const struct inotify_event* event) {
    filtered_files_.push_back(event->name);
    return true;
  }

  virtual bool DoFilterOnFilename(const char* filename) {
    filtered_files_.push_back(filename);
    return true;
  }

  deque<const char*> filtered_files_;
};

GTAGS_FIXTURE(IndexEventHandlerTest) {
 public:
  GTAGS_FIXTURE_SETUP(IndexEventHandlerTest) {
    watcher_.reset();
    q_ = new FilenamePCQueue(10);
    handler_ = new IndexEventHandler(&watcher_, q_);
    filter_.filtered_files_.clear();
    handler_->AddFilter(&filter_);
  }

  GTAGS_FIXTURE_TEARDOWN(IndexEventHandlerTest) {
    delete handler_;
    delete q_;
  }

 protected:
  class MockFileWatcher : public InotifyFileWatcher {
   public:
    virtual const char* GetPathname(int wd) {
      return "testpath";
    }

    virtual int GetWatchDescriptor(const char* pathname) {
      return 28;
    }

    virtual int AddDirectory(const char* name) {
      name_ = name;
      return 21;
    }

    virtual void RemoveDirectory(int wd) {
      wd_ = wd;
    }

    void reset() {
      wd_ = 0;
      name_ = 0;
    }

    int wd_;
    const char* name_;
  };

  MockFileWatcher watcher_;
  MockFilter filter_;
  IndexEventHandler* handler_;
  FilenamePCQueue* q_;
};

TEST_F(IndexEventHandlerTest, TestDoIndex) {
  struct inotify_event* event =
      CreateInotifyEvent(1, IN_CREATE, "test1");

  EXPECT_EQ(0, q_->count());
  handler_->HandleEvent(event);
  EXPECT_EQ(1, q_->count());
  char* filename = static_cast<char*>(q_->Get());
  EXPECT_STREQ("testpath/test1", filename);
  EXPECT_STREQ("test1", filter_.filtered_files_[0]);
  FreeInotifyEvent(event);
  delete[] filename;
}

TEST_F(IndexEventHandlerTest, TestHandleImport) {
  const char* filename_input = "testpath/test1";
  EXPECT_EQ(0, q_->count());
  handler_->HandleImport(filename_input);
  EXPECT_EQ(1, q_->count());
  char* filename = static_cast<char*>(q_->Get());
  EXPECT_STREQ("testpath/test1", filename);
  EXPECT_STREQ("testpath/test1", filter_.filtered_files_[0]);
  delete[] filename;
}

GTAGS_FIXTURE(DirectoryTrackerTest) {
 public:
  GTAGS_FIXTURE_SETUP(DirectoryTrackerTest) {
    watcher_.reset();
    tracker_ = new DirectoryTracker(&watcher_);
  }

  GTAGS_FIXTURE_TEARDOWN(DirectoryTrackerTest) {
    delete tracker_;
    tracker_ = 0;
  }

 protected:
  class MockFileWatcher : public InotifyFileWatcher {
   public:
    virtual const char* GetPathname(int wd) {
      return "testpath";
    }

    virtual int GetWatchDescriptor(const char* pathname) {
      return 28;
    }

    virtual int AddDirectory(const char* name) {
      name_ = name;
      return 21;
    }

    virtual void RemoveDirectory(int wd) {
      wd_ = wd;
    }

    void reset() {
      wd_ = 0;
      name_ = 0;
    }

    int wd_;
    const char* name_;
  };

  MockFileWatcher watcher_;
  DirectoryTracker* tracker_;
};

TEST_F(DirectoryTrackerTest, TestHandleCreate) {
  struct inotify_event* event =
      CreateInotifyEvent(1, IN_CREATE | IN_ISDIR, "test1");

  EXPECT_TRUE(watcher_.name_ == NULL);

  tracker_->HandleEvent(event);

  EXPECT_STREQ("testpath/test1", watcher_.name_);

  FreeInotifyEvent(event);
}

TEST_F(DirectoryTrackerTest, TestHandleDelete) {
  struct inotify_event* event =
      CreateInotifyEvent(1, IN_DELETE | IN_ISDIR, "test1");

  EXPECT_EQ(0, watcher_.wd_);

  tracker_->HandleEvent(event);

  EXPECT_EQ(28, watcher_.wd_);

  FreeInotifyEvent(event);
}


class InotifyFileWatcherTester : public InotifyFileWatcher {
 public:
  // expose a getter for test
  PathnameWatchDescriptorMap& map() {
    return map_;
  }

 protected:
  // Override all the system calls for unit testing
  virtual void Init() {}
  virtual int AddInotifyWatch(const char* dir_name) {
    static int seed = 1;
    return seed++;
  }
  virtual void RemoveInotifyWatch(int wd) {
  }
  virtual int ReadEvents() {
    return 1;
  }
};

GTAGS_FIXTURE(InotifyFileWatcherTest) {
 public:
  GTAGS_FIXTURE_SETUP(InotifyFileWatcherTest) {
    watcher_ = new InotifyFileWatcherTester();
    watcher_->AddEventHandler(&handler_);
  }

  GTAGS_FIXTURE_TEARDOWN(InotifyFileWatcherTest) {
    delete watcher_;
    handler_.imported.clear();
  }

 protected:
  InotifyFileWatcherTester* watcher_;
  MockInotifyEventHandler handler_;
};

TEST_F(InotifyFileWatcherTest, TestAddDirectory) {
  // Add a new dir
  int wd = watcher_->AddDirectory("dir1");
  EXPECT_EQ(wd, watcher_->map().GetWatchDescriptor("dir1"));

  // Add anotehr dir
  wd = watcher_->AddDirectory("dir2");
  EXPECT_EQ(wd, watcher_->map().GetWatchDescriptor("dir2"));

  // Add a dir that's already been added.
  int old_dir = watcher_->map().GetWatchDescriptor("dir1");
  wd = watcher_->AddDirectory("dir1");
  EXPECT_EQ(old_dir, wd);
}

TEST_F(InotifyFileWatcherTest, TestRemoveDirectory) {
  int wd1 = watcher_->AddDirectory("dir1");

  EXPECT_NE(0, watcher_->map().GetWatchDescriptor("dir1"));
  watcher_->RemoveDirectory(wd1);
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor("dir1"));

  // delete again.
  // should have no effect.
  watcher_->RemoveDirectory(wd1);
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor("dir1"));
}

TEST_F(InotifyFileWatcherTest, TestAddDirectoryRecursive) {
  string base_dir = TEST_DATA_DIR + "/test_dir";
  watcher_->AddDirectoryRecursive(base_dir.c_str());
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor(base_dir.c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir1").c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir2").c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir3").c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor(
                (base_dir + "/test_dir1/test_dir4").c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir5").c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir6").c_str()));
  EXPECT_EQ(2, handler_.imported.size());
  EXPECT_EQ(base_dir + "/test_dir1/test.h", handler_.imported[0]);
  EXPECT_EQ(base_dir + "/test.cc", handler_.imported[1]);
}

TEST_F(InotifyFileWatcherTest, TestRemoveDirectoryRecursive) {
  string base_dir = TEST_DATA_DIR + "/test_dir";
  watcher_->AddDirectoryRecursive(base_dir.c_str());
  watcher_->RemoveDirectoryRecursive(base_dir.c_str());
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor(base_dir.c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir1").c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir2").c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir3").c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor(
                (base_dir + "/test_dir1/test_dir4").c_str()));
}

TEST_F(InotifyFileWatcherTest, TestExcludes) {
  string base_dir = TEST_DATA_DIR + "/test_dir";
  watcher_->AddExcludeDirectory("test_dir1");
  watcher_->AddDirectoryRecursive(base_dir.c_str());
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir1").c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir2").c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir3").c_str()));
  EXPECT_EQ(0, watcher_->map().GetWatchDescriptor(
                (base_dir + "/test_dir1/test_dir4").c_str()));
  watcher_->RemoveExcludeDirectory("test_dir1");
  watcher_->AddDirectoryRecursive(base_dir.c_str());
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor((base_dir +
                                                   "/test_dir1").c_str()));
  EXPECT_NE(0, watcher_->map().GetWatchDescriptor(
                (base_dir + "/test_dir1/test_dir4").c_str()));

}

}  // namespace
