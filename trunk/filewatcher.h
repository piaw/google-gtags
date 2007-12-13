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

#ifndef TOOLS_TAGS_FILEWATCHER_H__
#define TOOLS_TAGS_FILEWATCHER_H__

#include <sys/syscall.h>

#include <cstring>
#include <list>
#include <ext/hash_map>
#include <ext/hash_set>

#include "inotify.h"
#include "inotify-syscalls.h"

#include "logging.h"
#include "mutex.h"

namespace gtags {
template<typename T> class ProducerConsumerQueue;
}
typedef gtags::ProducerConsumerQueue<char*> FilenamePCQueue;
using gtags::Mutex;

const int kMaxPathLength = 512;

// Join paths together, with '/' added if necessary.
//
// joined_name is the buffer the result will write to. It is assumed to be large
// enough to hold kMaxPathLength characters.
const char* JoinPath(const char* basepath,
                     const char* childpath,
                     char* joined_name);

// PathnameWatchDescriptorMap is a two way map between a pathname and a wd
// (watch descritpor). This class is threadsafe.
class PathnameWatchDescriptorMap {
 public:
  PathnameWatchDescriptorMap() {}
  virtual ~PathnameWatchDescriptorMap();

  // Add pathname and wd to approperiate associative containers.
  // This method makes a copy of pathname.
  virtual void Add(const char* pathname, int wd);

  // Given a wd, remove an entry associated from the mappings.
  // This would deallocate the char array storing the filename.
  virtual void Remove(int wd);

  // Given a pathname, return the associated wd. Returns 0 if pathname is not
  // in the map.
  virtual int GetWatchDescriptor(const char* pathname);

  // Given a wd, return the associated pathname. Returns NULL if wd is not in
  // the map.
  virtual const char* GetPathname(int wd);

  // Given a directory, output all watched directories that belong inside it.
  virtual void GetSubDirs(const char* pathname, list<const char*>* output);
  // Given a directory, output all WD of watched directories that belong inside
  // it.
  virtual void GetSubDirsWatchDescriptor(const char* pathname,
                                         list<int>* output);

 protected:
  virtual const char* GetPathnameNoLock(int wd);

 private:
  class StrEq {
   public:
    bool operator()(const char* s1, const char* s2) const {
      return !strcmp(s1, s2);
    }
  };

  // Short hands for long type names.
  typedef hash_map<const char*, int, hash<const char*>, StrEq> CStringIntMap;
  typedef hash_map<int, const char*> IntCStringMap;

  CStringIntMap pathname_to_wd_map_;
  IntCStringMap wd_to_pathname_;
  Mutex mutex_;
};

class InotifyEventFilter;

// InotifyEventHandler is the base class for all event handlers that can
// receive inotify kernel events. Implementation class should override one of
// the Handle... function for specific events they wish to receive.
class InotifyEventHandler {
 public:
  InotifyEventHandler() {}
  virtual ~InotifyEventHandler() {}
  virtual void HandleEvent(struct inotify_event* event);

  virtual void AddFilter(InotifyEventFilter* filter) {
    filters_.push_back(filter);
  }

  // Override one of the following functions to handle specific events.
  virtual void HandleAccess(struct inotify_event* event) {}
  virtual void HandleModify(struct inotify_event* event) {}
  virtual void HandleAttrib(struct inotify_event* event) {}
  virtual void HandleCloseWrite(struct inotify_event* event) {}
  virtual void HandleCloseNoWrite(struct inotify_event* event) {}
  virtual void HandleOpen(struct inotify_event* event) {}
  virtual void HandleMovedFrom(struct inotify_event* event) {}
  virtual void HandleMovedTo(struct inotify_event* event) {}
  virtual void HandleCreate(struct inotify_event* event) {}
  virtual void HandleDelete(struct inotify_event* event) {}

  // FileWatcher calls this function during its scan for new files during
  // AddDirectoryRecursive. Override this function when files new to the
  // watcher need to be handled in someway. Note that new here means new
  // to the watcher, not new to the file system.
  virtual void HandleImport(const char* filename) {}

 protected:
  list<InotifyEventFilter*> filters_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(InotifyEventHandler);
};

// A simple event logger.
class InotifyEventLogger: public InotifyEventHandler {
 public:
  InotifyEventLogger() {}

  virtual ~InotifyEventLogger() {}

  virtual void HandleAttrib(struct inotify_event* event) {
    LOG(INFO) << event->name << " attribute changed.";
  }

  virtual void HandleModify(struct inotify_event* event) {
    LOG(INFO) << event->name << " modified.";
  }

  virtual void HandleCreate(struct inotify_event* event) {
    LOG(INFO) << event->name << " created.";
  }

  virtual void HandleDelete(struct inotify_event* event) {
    LOG(INFO) << event->name << " deleted.";
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(InotifyEventLogger);
};

// Forward declaration
class InotifyFileWatcher;

// IndexEventHandler listens to modify, create and delete events and push
// changed files on a PCQueue. A separate thread runs on the receiving end of
// the queue and forward these files to the indexer.
class IndexEventHandler : public InotifyEventHandler{
 public:
  IndexEventHandler(InotifyFileWatcher* watcher, FilenamePCQueue* queue)
      :watcher_(watcher), queue_(queue) {}
  virtual ~IndexEventHandler() {}

  virtual void HandleModify(struct inotify_event* event) {
    DoIndexing(event);
  }

  virtual void HandleCreate(struct inotify_event* event) {
    DoIndexing(event);
  }

  virtual void HandleDelete(struct inotify_event* event) {
    DoIndexing(event);
  }

  virtual void HandleImport(const char* filename);

 protected:
  void DoIndexing(struct inotify_event* event);

 private:
  InotifyFileWatcher* watcher_;
  FilenamePCQueue* queue_;

  DISALLOW_EVIL_CONSTRUCTORS(IndexEventHandler);
};

// InotifyEventFilter is the base class for all event filters. An event filter
// can be added to one or more InotifyEventHandlers to intercepts inotify_events
// before event is delivered to specific event handling functions.
//
// Subclasses of InotifyEventFilter must override DoFilter which returns true
// if the event should be handled by the handler or false if the event should be
// ignored.
class InotifyEventFilter {
 public:
  InotifyEventFilter() {}

  virtual ~InotifyEventFilter() {}

  virtual bool DoFilter(const struct inotify_event* event) = 0;
  virtual bool DoFilterOnFilename(const char* filename) = 0;

 private:

  DISALLOW_EVIL_CONSTRUCTORS(InotifyEventFilter);
};

// InotifyDirectoryEventHandler is only interested in events happening to a
// directory.
class DirectoryEventFilter : public InotifyEventFilter {
 public:
  DirectoryEventFilter() {}

  virtual ~DirectoryEventFilter() {}

  virtual bool DoFilter(const struct inotify_event* event) {
    // Only dispatch events on directory.
    return event->mask & IN_ISDIR;
  }

  virtual bool DoFilterOnFilename(const char* filename) {
    return true;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DirectoryEventFilter);
};

// An event filter using a white-list of file extensions.
class FileExtensionEventFilter : public InotifyEventFilter {
 public:
  FileExtensionEventFilter() {}

  virtual ~FileExtensionEventFilter() {}

  void AddExtension(const char* ext) {
    white_listed_extensions_.insert(ext);
  }

  virtual bool DoFilter(const struct inotify_event* event) {
    if (event->len == 0) { // no file name
      return false;
    }
    return DoFilterOnFilename(event->name);
  }

  virtual bool DoFilterOnFilename(const char* filename) {
    const char* ext = rindex(filename, '.');
    if (ext == NULL) {
      return false;
    }

    return white_listed_extensions_.find(ext) != white_listed_extensions_.end();
  }

 private:
  class StrEq {
   public:
    bool operator()(const char* s1, const char* s2) const {
      return !strcmp(s1, s2);
    }
  };

  hash_set<const char*, hash<const char*>, StrEq> white_listed_extensions_;

  DISALLOW_EVIL_CONSTRUCTORS(FileExtensionEventFilter);
};

// An event filter using a black-listed prefix.
class PrefixFilter : public InotifyEventFilter {
 public:
  PrefixFilter() {}

  virtual ~PrefixFilter() {}

  virtual bool DoFilter(const struct inotify_event* event) {
    return DoFilterOnFilename(event->name);
  }

  virtual bool DoFilterOnFilename(const char* filename) {
    return filename[0] != '.' && filename[0] != '~'
        && filename[0] != '#';
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(PrefixFilter);
};

// DirectoryTrack keeps our directory watch list up to date. For each monitored
// directory, we update the watch map on events concerning subdirectories.
class DirectoryTracker : public InotifyEventHandler {
 public:
  DirectoryTracker(InotifyFileWatcher* watcher) : watcher_(watcher) {
  }

  virtual ~DirectoryTracker() {}

  virtual void HandleCreate(struct inotify_event* event);
  virtual void HandleDelete(struct inotify_event* event);

 private:
  InotifyFileWatcher* watcher_;
  char join_name_buffer_[kMaxPathLength];
  DISALLOW_EVIL_CONSTRUCTORS(DirectoryTracker);
};

// size of a inotify_event structure.
// Note that inotify_event contains a dynamic sized character array.
// kInotifyEventSize does not include the size of this array. The size of the
// dynamic array is given by inotify_event.len.
const int kInotifyEventSize = sizeof (struct inotify_event);

// This is roughly the size of 1024 inotify_events including their dynamic
// sized filenames. Assuming average length of the filenames relative to
// dir_name_ is 32.
// The exact number of inotify_events it contains is not important as long as
// we are confident that it is large enough.
const int kBufferSize = 1024 * (kInotifyEventSize + 32);

class InotifyFileWatcher {
 public:
  InotifyFileWatcher() {}

  virtual ~InotifyFileWatcher() {
    close(fd_);
  }

  void Loop();

  virtual int AddDirectory(const char* dir_name);

  virtual void AddDirectoryRecursive(const char* dir_name);

  virtual void RemoveDirectory(int wd);

  virtual void RemoveDirectoryRecursive(const char* dir_name);

  void AddEventHandler(InotifyEventHandler* event_handler) {
      event_handlers_.push_back(event_handler);
  }

  void RemoveEventHandler(InotifyEventHandler* event_handler) {
    event_handlers_.remove(event_handler);
  }

  // Given a pathname, return the associated wd. Returns 0 if pathname is not
  // in the map.
  virtual int GetWatchDescriptor(const char* pathname) {
    return map_.GetWatchDescriptor(pathname);
  }

  // Given a wd, return the associated pathname. Returns NULL if wd is not in
  // the map.
  virtual const char* GetPathname(int wd) {
    return map_.GetPathname(wd);
  }

  // Add directory name to ignore for all add/remove operations.
  virtual void AddExcludeDirectory(const char* dir);

  // Remove directory name to ignore for all add/remove operations.
  virtual void RemoveExcludeDirectory(const char* dir);

 protected:
  virtual void Init();

  virtual int AddInotifyWatch(const char* dir_name) {
    return inotify_add_watch(fd_, dir_name, IN_ALL_EVENTS);
  }

  virtual void RemoveInotifyWatch(int wd) {
    inotify_rm_watch(fd_, wd);
  }

  virtual int ReadEvents() {
    return read(fd_, event_buffer_, kBufferSize);
  }

  // Read a bunch of inotify events, iterate through them and process each one.
  virtual void ProcessEvents();

  // Provide a string operator== for hashed string containers
  class StrEq {
   public:
    bool operator()(const char* s1, const char* s2) const {
      return !strcmp(s1, s2);
    }
  };

  // File descriptor for inotify
  int fd_;
  char event_buffer_[kBufferSize];
  PathnameWatchDescriptorMap map_;
  list<InotifyEventHandler*> event_handlers_;
  hash_set<const char*, hash<const char*>, StrEq> exclude_list_;

 private:
  // buffer for JoinName.
  char join_name_buffer_[kMaxPathLength];
};

#endif  // TOOLS_TAGS_FILEWATCHER_H__
